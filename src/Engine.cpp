#include "Engine.hpp"

void VG::Engine::setDefaultSettings(Settings settings){
	/*
	These are the values that will be used in case a settings file is missing info.
	*/

	m_default_settings = settings;
}

void VG::Engine::loadSettingsFromFile(std::string filepath){
	/*
	Loads settings from the given file. Any required values that are
	not defined default to the contents of m_default_settings.
	*/

	Settings defaults = m_default_settings;
	auto [is_valid, loaded_settings] = FileIO::loadSettingsFromFile(filepath);
	if(is_valid){
		loaded_settings.fillUndefinedValuesFromReference(defaults);
		m_settings_ptr = std::make_shared<Settings>(loaded_settings);
	
		m_chunk_manager->updateWithSettings(m_settings_ptr);
	}else{
		// TODO: Proper error handling
		assert(false);
	}
}

//-------------------------------------------------------------------------------------------------
// Engine
//-------------------------------------------------------------------------------------------------
VG::Engine::Engine(){
	m_default_settings = defaultSettings();
	m_settings_ptr = std::make_shared<Settings>(m_default_settings);
	
	m_simcache.m_resource_manager = new ResourceManager();
	m_chunk_manager = new ChunkManager();
}

VG::Engine::~Engine(){
	delete m_chunk_manager;
}

void VG::Engine::setTargetWorld(WorldState* world_ptr){
	assert(world_ptr != NULL);

	m_simcache.m_reference_world = world_ptr;
	m_chunk_manager->setManagedTable(world_ptr->m_chunk_table);
}

void VG::Engine::initWindow(){
	if(m_settings_ptr->namespaceRef("ENGINE")["UseGraphics"].val_bool){	
		printf("Engine: The engine is running with graphics enabled\n");
		m_window_ptr = std::shared_ptr<Window>(new Window());
		m_window_ptr->initWindow();
		if(!m_window_ptr->isValid()){
			printf("ERROR: Window initialization failed!\n");
			m_should_run = false;
		}
	}
	else{
		printf("Engine: The engine is running in headless mode\n");
		m_window_ptr = std::shared_ptr<Window>(NULL);
	}
}

void VG::Engine::initTargetWorld(){
	/*

	*/

	assert(m_simcache.m_reference_world != NULL);
	// Generate chunks around the world origin
	IVec3& bounds_min = m_settings_ptr->namespaceRef("CHUNK_GEN")["MinBounds"].val_ivec3;
	IVec3& bounds_max = m_settings_ptr->namespaceRef("CHUNK_GEN")["MaxBounds"].val_ivec3;
	IVec3 delta = {
		(bounds_max.x - bounds_min.x) + 1,
		(bounds_max.y - bounds_min.y) + 1,
		(bounds_max.z - bounds_min.z) + 1,
	};
	for(int z = bounds_min.z; z <= bounds_max.z; ++z)
	for(int y = bounds_min.y; y <= bounds_max.y; ++y)
	for(int x = bounds_min.x; x <= bounds_max.x; ++x){
		ChunkInstruction instruction = {CHUNK_GENERATE, {x, y, z}};
		m_chunk_manager->addInstruction(instruction);
	}

	Widget bounds_widget = {
		.type=WIDGET_CUBOID,
		.cuboid={
			.origin=toFloatVector(bounds_min) * CHUNK_LEN, 
			.extent=toFloatVector(delta) * CHUNK_LEN,
			.color={1, 1, 0.1}
		}
	};
	WorldState* world_ptr = m_simcache.m_reference_world;
	world_ptr->addWidgetData("Landmarks", {bounds_widget}, true);

	Int64 num_chunks = delta.x * delta.y * delta.z;
	printf("Engine: Generating %li chunks...\n", num_chunks);
	m_chunk_manager->processInstructions();
	printf("Engine: Done.\n");


	// KDTree settings
	const ChunkTable& table = m_simcache.m_reference_world->m_chunk_table;
	ICuboid chunkspace_bounds = table.boundingVolumeChunkspace();
	
	Settings::Namespace tree_settings = m_settings_ptr->namespaceRef("VKDTREE");
	VoxelKDTree::BuildSettings settings;
	settings.max_depth = tree_settings["MaxDepth"].val_int;
	settings.mandatory_leaf_volume = tree_settings["MandatoryLeafVolume"].val_int;
	settings.bounds = {
		chunkspace_bounds.origin * CHUNK_LEN,
		chunkspace_bounds.extent * CHUNK_LEN,
	};
	m_simcache.generateAccelerationStructures(settings);
}

void VG::Engine::initResourceData(std::string filepath){
	/*
	Given the filepath of a resource declaration file, load as many of the
	declared resources as possible into the resource manager.

	TODO: Move this into the resource manager
	*/

	std::vector<SystemInstruction> resource_update_instructions;
	std::vector<FileIO::ResourceDeclaration> declarations = FileIO::loadResourceDeclarations(filepath);
	for(auto decl : declarations){
		auto [name, type, source_filepath] = decl;

		if(type == RESOURCE_RIGID_TRIANGLE_MESH){
			auto [is_success, mesh] = MeshLoader::Obj::loadFromFile(source_filepath);
			if(is_success){
				ResourceHandle new_handle = m_simcache.m_resource_manager->addResource(name, mesh);

				SystemInstruction instruction = {
					.type=INSTRUCTION_ASSET,
					.asset_instruction={
						.type=LOAD_ASSET_BY_ID,
						.handle=new_handle
					}
				};
				resource_update_instructions.push_back(instruction);
			}else{
				// Error
			}
		}else{
			// Error
			// printf("\tUnable to load assets of type '%s'\n", RESOURCE_STRINGS[type]);
		}
	}

	for(auto instruction : resource_update_instructions){
		m_renderer.sendInstruction(instruction);
	}
}

void pause(double seconds){
	if(seconds < 0) return;

	int microseconds = seconds * 1e6;
	usleep(microseconds);
}

int sign(int value){
	if(value < 0){
		return -1;
	}else{
		return 1;
	}
}

std::vector<UnitOrder> ordersFromInputs(std::vector<InputEvent> input_events){
	/*
	Given a list of input events, return a list of orders for the player. Attempts to handle
	multiple conflicting inputs for movement. Also attempts to avoid summing movement if multiple
	controllers are running simultaneously. IE: Controller + keyboard sending "run forward" and
	"walk forward" commands respectively shouldn't result in the player getting a speed boost, or
	sending "run forward" and "walk forward" orders at the same time.

	WARNING: Multiple inputs with conflicts will still produce bugs.
		TODO: Deal with SPACE + Action commands
	TODO: Allow for remapping of controls. Possibly have a ControlMap object or the like.
	*/

	std::vector<UnitOrder> output_orders;

	IVec3 RIGHT =   {1, 0, 0};
	IVec3 FORWARD = {0, 1, 0};
	IVec3 UP =      {0, 0, 1};

	// Map keys to preset orders (orders with either no data or consistent data)
	// Not putting them in the map directly because that causes gnarly template errors.
	UnitOrder default_jump_order =        {ORDER_JUMP, .jump_order={JUMP_VERTICAL,}};
	UnitOrder default_interaction_order = {ORDER_INTERACT, };
	std::unordered_map<Key, UnitOrder> key_to_order = {
		{KEY_SPACE,       default_jump_order},
		{KEY_I, default_interaction_order}
	};
	
	// Map keys to translation
	IVec3 translations_requested = {0, 0, 0};
	std::unordered_map<Key, IVec3> key_to_translation = {
		{KEY_D, RIGHT},    {KEY_A, -RIGHT},     // Translate X
		{KEY_W, FORWARD},  {KEY_S, -FORWARD},   // Translate Y
		{KEY_E, UP},       {KEY_Q, -UP},        // Translate Z
	};

	// Map keys to rotation
	IVec2 rotations_requested = {0, 0};
	std::unordered_map<Key, IVec2> key_to_rotation = {
		{KEY_LEFT, {1, 0}}, {KEY_RIGHT, {-1, 0}},  // Rotate Horizontal
		{KEY_UP,   {0, 1}}, {KEY_DOWN,  {0, -1}}   // Rotate Vertical
	};

	// STEP 1: Sum requested translations/rotations in "movement_requested" vectors to cancel out
	// conflicting movement orders. IE: Holding forward and back simultaneously. In a bizarre
	// scenario of somebody hitting multiple movement inputs across different controller types
	bool is_boosting = false;
	for(InputEvent event : input_events){
		if(event.source_type == InputSourceType::DEVICE_KEYBOARD){
			KeyboardEvent key_event = event.keyboard;
			if(key_event.event_type == KeyEventType::KEY_PRESSED){
				// Boosting is applied once to the net movement vector. Not applied here.
				if(key_event.keycode == KEY_LEFT_SHIFT || key_event.keycode == KEY_RIGHT_SHIFT){
					is_boosting = true;
				}

				// Handle preset orders
				auto iter1 = key_to_order.find((Key)key_event.keycode);
				if(iter1 != key_to_order.end()){
					UnitOrder new_preset_order = iter1 -> second;
					output_orders.push_back(new_preset_order);
				}

				// Handle translation
				auto iter2 = key_to_translation.find((Key)key_event.keycode);
				if(iter2 != key_to_translation.end()){
					translations_requested += iter2 -> second;
				}

				// Handle rotation
				auto iter3 = key_to_rotation.find((Key)key_event.keycode);
				if(iter3 != key_to_rotation.end()){
					rotations_requested += iter3 -> second;
				}
				
			}else if(key_event.event_type == KeyEventType::KEY_HELD){
				// Nothing for now
			}else if(key_event.event_type == KeyEventType::KEY_RELEASED){
				// Nothing for now
			}
		}
	}

	// STEP 2: Go through the displacement and rotation variables.
	float EPSILON = 0.0001;

	// Generate a movement order
	FVec3 desired_translation_dir = {0, 0, 0};
	for(int i = 0; i < 3; ++i){
		int net_diff = translations_requested[i];
		if(net_diff != 0){
			desired_translation_dir[i] = 1 * sign(net_diff);
		}
	}
	if(desired_translation_dir.length() > EPSILON){
		FVec3 translation_vector = desired_translation_dir.normal();
		UnitOrderType translation_type = is_boosting ? ORDER_TRANSLATE_RUN : ORDER_TRANSLATE_WALK;
		UnitOrder order = {translation_type, .translation_order={translation_vector}};
		output_orders.push_back(order);
	}

	// Rotation amount
	FVec2 desired_rotation_amounts = {0, 0};
	for(int i = 0; i < 2; ++i){
		int net_diff = rotations_requested[i];
		if(net_diff != 0){
			desired_rotation_amounts[i] = 1 * sign(net_diff);
		}
	}
	if(desired_rotation_amounts.length() > EPSILON){
		FVec2 rotation_vector = desired_rotation_amounts.normal();
		UnitOrder order = {ORDER_ROTATE_ELEMENTWISE, .rotation_order={rotation_vector}};
		output_orders.push_back(order);
	}

	return output_orders;
}

void assertNoRedundantOrders(std::vector<UnitOrder> order_list){
	/*
	Counts up the different types of orders in a given list and asserts that there are not
	multiple orders of the same type.
	*/

	// Iterate over the list and count up orders by type. Note that some different order types
	// fall into the same category.
	constexpr int INDEX_MOVE_COUNT = 0;
	constexpr int INDEX_ROTATION_COUNT = 1;
	constexpr int INDEX_JUMP_COUNT = 2;
	constexpr int INDEX_SHOOT_COUNT = 3;
	constexpr int INDEX_INTERACTION_COUNT = 4;

	int order_counts[] = {0, 0, 0, 0, 0};
	for(UnitOrder order : order_list){
		switch(order.type){
			case ORDER_TRANSLATE_CROUCH_WALK:
				order_counts[INDEX_MOVE_COUNT] += 1;
				break;
			case ORDER_TRANSLATE_WALK:
				order_counts[INDEX_MOVE_COUNT] += 1;
				break;
			case ORDER_TRANSLATE_RUN:
				order_counts[INDEX_MOVE_COUNT] += 1;
				break;
			case ORDER_ROTATE_ELEMENTWISE:
				order_counts[INDEX_ROTATION_COUNT] += 1;
				break;
			case ORDER_LOOK_AT:
				order_counts[INDEX_ROTATION_COUNT] += 1;
				break;
			case ORDER_JUMP:
				order_counts[INDEX_JUMP_COUNT] += 1;
				break;
			case ORDER_SHOOT:
				order_counts[INDEX_SHOOT_COUNT] += 1;
				break;
			case ORDER_INTERACT:
				order_counts[INDEX_INTERACTION_COUNT] += 1;
				break;
			default:
				printf("Unknown order type: %i\n", order.type);
				assert(false);
		};
	}

	// Now assert that we didn't get more than one of each order type.
	int num_redundancies = false;
	for(int i = 0; i < 5; ++i){
		if(order_counts[i] > 1){
			num_redundancies += 1;
		}
	}
	assert(num_redundancies == 0);
}

Basis rotatePlayerBasis(Basis basis, float horizontal, float vertical){
	/*
	In order to prevent the view from abnormal rotations, the player rotates
	horizontally around the global vertical axis instead of their local one.
	
	NOTE: If the player is upside-down, the vertical axis swaps to
	prevent the rotational controls from becoming inverted.
	*/

	Basis output_basis = basis;
	constexpr FVec3 VERTICAL_AXIS = {0, 0, 1};
	constexpr FVec3 INVERTED_VERTICAL_AXIS = {0, 0, -1};
	bool should_invert_controls_upside_down = false;

	// Rotate Horizontal
	if(horizontal != 0){
		FVec3 rotation_axis = VERTICAL_AXIS;
		if(basis.v2.dot(rotation_axis) < 0 && should_invert_controls_upside_down){
			rotation_axis = INVERTED_VERTICAL_AXIS;
		}

		output_basis.v0 = output_basis.v0.rotation(rotation_axis, horizontal);
		output_basis.v1 = output_basis.v1.rotation(rotation_axis, horizontal);
	}

	// Rotate Vertical
	if(vertical != 0){
		output_basis.v2 = output_basis.v2.rotation(output_basis.v0, vertical);
		output_basis.v1 = output_basis.v1.rotation(output_basis.v0, vertical);
	}

	return output_basis;
}

PlaceholderEntity handlePlayerOrders(PlaceholderEntity curr_player, std::vector<UnitOrder> player_orders, double dt){
	/*
	WARNING: Temporary function for the sake of getting visuals working. 
	WARNING: This function assumes that the player is noclipping around. Ignores physics.
	TODO: Once a proper entity sim system is in place, this needs to be refactored and moved there.
	*/

	assert(curr_player.handle.type_index == (Uint32) EntityType::ENTITY_PLAYER);
	assertNoRedundantOrders(player_orders);

	constexpr float PLAYER_WALK_SPEED = 10.0;
	constexpr float PLAYER_RUN_SPEED = 400.0;
	constexpr float ROTATION_SPEED = 100.0;

	PlaceholderEntity updated_player = curr_player;
	for(UnitOrder order : player_orders){
		if(order.type == ORDER_TRANSLATE_WALK || order.type == ORDER_TRANSLATE_RUN){
			// TODO: Fix jank
			float speed = order.type == ORDER_TRANSLATE_WALK ? PLAYER_WALK_SPEED : PLAYER_RUN_SPEED;
			speed *= dt;
			FVec3 displacement = order.translation_order.relative_amounts.normal() * speed;
			
			FVec3 player_displacement = {0, 0, 0};
			player_displacement += curr_player.basis.v0 * displacement.x;
			player_displacement += curr_player.basis.v1 * displacement.y;
			player_displacement += curr_player.basis.v2 * displacement.z;
			updated_player.position += player_displacement;
		}else if(order.type == ORDER_ROTATE_ELEMENTWISE){
			FVec2 rotation = order.rotation_order.relative_amounts * ROTATION_SPEED * dt;
			Basis updated_basis = rotatePlayerBasis(updated_player.basis, 
				rotation.x, rotation.y);
			updated_player.basis = updated_basis.orthogonalized();
		}
	}

	return updated_player;
}

Camera snapCameraToPlayerView(Camera camera, PlaceholderEntity player){
	/*
	Given a camera, snap its position and basis to match the player's
	position and look direction.
	*/

	Camera output_camera = camera;
	output_camera.pos = player.position;
	output_camera.basis = player.basis;

	return output_camera;
}

std::vector<ChunkInstruction> chunkMeshingInstructions(const SimCache* state){
	/*
	Helper function to generate a list of chunk meshing instructions for the given simulation state
	*/

	std::vector<ChunkInstruction> generation_instructions;

	Leniency leniency = 1;

	// Stupid selection strategy. Create a mesh for every unmeshed loaded chunk.
	std::vector<IVec3> all_loaded = state->m_reference_world->m_chunk_table.allLoadedChunks();
	for(IVec3 chunk_addr : all_loaded){
		CellAddress cell_addr = {.corner_addr=chunk_addr, .lod_power=0};

		// See if this exists already and only generate chunks that don't have meshes.
		if(!state->m_chunk_lod_meshes.cellData(cell_addr).is_valid){
			ChunkInstruction new_instruction = {CHUNK_GENERATE_UNLIT_MESH, cell_addr, leniency};
			generation_instructions.push_back(new_instruction);
		}
	}

	return generation_instructions;
}

void VG::Engine::useProvidedSettings(Settings settings){
	/*
	Given a settings object, update the internal settings object and send
	notifications to all subordinate systems that the update occurred. 
	*/

	m_settings_ptr = std::make_shared<Settings>(settings);
	m_chunk_manager->updateWithSettings(m_settings_ptr);
	
	// TODO: Update step. Possibly pass them a weak_ptr

}

void VG::Engine::updateWidgetAssets(Renderer& world_renderer){
	/*
	After changes to any of the world's widget groups, this is called to synchronize
	any unprocessed data with the asset manager.
	*/
	
	WorldState* world_ptr = m_simcache.m_reference_world;
	std::vector<std::string> all_group_names = world_ptr->allGroupNames();
	for(std::string& name : all_group_names){
		assert(name.size() <= PODString::MAX_LEN);
		const WidgetGroup& data = world_ptr->groupData(name);

		Fingerprint last_seen = m_widget_fingerprints[name];
		Fingerprint current = data.fingerprint;
		if(current != last_seen){
			world_renderer.sendInstruction(SystemInstruction{
				.type=INSTRUCTION_ASSET,
				.asset_instruction={
					.type=LOAD_ASSET_BY_NAME,
					.name=PODString::init(name.c_str())
				}
			});
		}
		m_widget_fingerprints[name] = current;
	}
}

Widget intersectionWidget(Ray ray, RayIntersection intersection){
	constexpr FVec2 FAKE_UV = {0, 0};

	bool full_len = isValid(intersection);
	float length = full_len ? intersection.t_hit : 1000.0;
	FVec3 end_pos = ray.origin + ray.dir.normal() * length;
 
 	FVec3 color = {1, 0, 0};
	if(intersection.type == INTERSECT_POSSIBLE_CHUNK_VOXEL || intersection.type == INTERSECT_HIT_CHUNK_VOXEL){
		color = {1, 1, 1};
	}

	Widget new_hit_widget = {
		.type=WIDGET_LINE,
		.line={
			.v1={ray.origin, FAKE_UV, color}, 
			.v2={end_pos, FAKE_UV, color}
		}
	};

	return new_hit_widget;
}

Raytracer::RenderSettings initRenderSettings(std::shared_ptr<Settings> ptr){
	/*
	This is currently called in two places and is mostly standalone
	*/

	auto ray_settings = ptr->namespaceRef("RAYTRACING");
	Rendering::ImageConfig image_config = {
		.num_pixels=ray_settings["ImageDimensions"].val_ivec2,
		.tile_dims=ray_settings["TileDimensions"].val_ivec2,
	};

	FVec3 sky_brightness = ray_settings["SkyColor"].val_fvec3 * 
		ray_settings["SkyBrightnessMultiplier"].val_float;
	FVec3 sun_brightness = ray_settings["SunColor"].val_fvec3 * 
		ray_settings["SunBrightnessMultiplier"].val_float;
	FVec3 sun_dir = ray_settings["SunDirection"].val_fvec3;

	Raytracer::RenderSettings render_settings = {
		.image_config=image_config,

		.num_render_threads=ray_settings["NumRenderThreads"].val_int,
		.num_rays_per_pixel=ray_settings["RaysPerPixel"].val_int,
		.max_path_len=ray_settings["MaxPathLen"].val_int,

		.sky_brightness=sky_brightness,
		.sun_brightness=sun_brightness,
		.sun_direction=sun_dir.normal(),
	};

	return render_settings;
}

FVec3 randomComponents(MathUtils::Random::SebVignaSplitmix64& splitmix){
	FVec3 random_vec{
		(float) splitmix.next(), 
		(float) splitmix.next(), 
		(float) splitmix.next()
	};
	return random_vec.normal();
}

void VG::Engine::runMainLoop(){
	/*
	Runs the game. 

	REFACTOR: Self-evident
	*/

	// Either we need a window or headless mode
	assert(m_window_ptr || !m_settings_ptr->namespaceRef("ENGINE")["UseGraphics"].val_bool);
	m_renderer.initOpenGL();  // Has to happen after opengl init

	// TODO: Remove this step once all world references are refactored to run via ptr
	WorldState* world_state = m_simcache.m_reference_world;

	Raytracer raytracer;
	raytracer.setWindowPtr(m_window_ptr);
	Camera camera = initDefaultCamera();
	PlaceholderEntity& player = world_state->m_placeholder_entities[0];
	assert(player.handle.type_index == (Uint32) EntityType::ENTITY_PLAYER);

	m_window_ptr->setClearColor(world_state->m_fog_color);
	m_window_ptr->setFullscreenState(
		m_settings_ptr->namespaceRef("WINDOW")["ShouldStartFullscreen"].val_bool);

	for(ChunkInstruction meshing_instruction : chunkMeshingInstructions(&m_simcache)){
		SystemInstruction sys_meshing_instruction = {
			.type=INSTRUCTION_CHUNK,
			.chunk_instruction=meshing_instruction
		};
		m_renderer.sendInstruction(sys_meshing_instruction);
	}

	float debounce_time = 0;  // Time since last key input
	bool rendermode_index = 0;
	PODString FACE_RENDERING_OPTIONS[] = {
		RENDERER_LINES_COMMAND_STRING,
		RENDERER_FACES_COMMAND_STRING,
	};

	bool widget_render_index = 0;
	PODString WIDGET_RENDERING_OPTIONS[] = {
		RENDERER_HIDE_WIDGETS,
		RENDERER_SHOW_WIDGETS,		
	};

	bool should_render_widgets = 
		m_settings_ptr->namespaceRef("ENGINE")["ShouldRenderWidgets"].val_bool;
	SystemInstruction initial_widget_render_setting = {
		.type=INSTRUCTION_GENERAL_TEXT,
		.text_instruction={WIDGET_RENDERING_OPTIONS[should_render_widgets]}
	};
	m_renderer.sendInstruction(initial_widget_render_setting);

	// For screenshots vs renders
	MathUtils::Random::SebVignaSplitmix64 random{3141592};
	bool should_raytrace = false;
	Raytracer::RenderSettings render_settings = initRenderSettings(m_settings_ptr);
	Rendering::ImageConfig& image_config = render_settings.image_config;
	
	// Need to get initial widgets rendering
	updateWidgetAssets(m_renderer);

	// Switches to control what parts of the main loop update
	bool should_exit = false;
	bool should_execute_simulation = true;

	auto& engine_namespace = m_settings_ptr->namespaceRef("ENGINE");
	auto& input_namespace = m_settings_ptr->namespaceRef("INPUT");
	Int32 target_fps = engine_namespace["TargetFPS"].val_int;
	float debounce_window = input_namespace["KeyDebounceTimeSeconds"].val_float;
	double target_frametime = 1.0 / target_fps;
	printf("Engine: Running at target fps of %i (%f seconds per frame)\n", 
		target_fps, target_frametime);
	Uint64 num_cycles = 0;
	while(!should_exit){
		// WARNING: Assumes all inputs are provided through the window
		world_state->addDeltaT(target_frametime);
		if(m_window_ptr){
			m_window_ptr->clear();
			m_window_ptr->pollEvents();
			IVec2 window_dims = m_window_ptr->getWindowDimensions();
			camera.aspect_ratio = (float) window_dims.x / (float) window_dims.y;

			std::vector<InputEvent> inputs = getInputEvents();
			std::vector<UnitOrder> orders = ordersFromInputs(inputs);
			player = handlePlayerOrders(player, orders, target_frametime);
			camera = snapCameraToPlayerView(camera, player);

			//-----------------------------------
			// Check for key commands
			//-----------------------------------
			std::unordered_set<Key> pressed_keys;
			for(const InputEvent event : inputs){
				if(event.source_type == InputSourceType::DEVICE_KEYBOARD){
					auto keycode = event.keyboard.keycode;
					pressed_keys.insert((Key) keycode);
				}
			}

			// TODO: Do this in input handling instead of here
			bool is_shift_pressed = pressed_keys.count(KEY_LEFT_SHIFT) || pressed_keys.count(KEY_RIGHT_SHIFT);
			if(is_shift_pressed){
				if(pressed_keys.count(KEY_EQUALS_SIGN)){
					pressed_keys.erase(KEY_EQUALS_SIGN);
					pressed_keys.insert(KEY_PLUS_SIGN);
				}
			}

			// TODO: Move all these into a separate function
			bool renderer_should_update = false;
			if(pressed_keys.count(KEY_ESCAPE)){
				should_exit = true;
			}

			if(pressed_keys.count(KEY_DELETE)){
				should_exit = true;
			}

			if(pressed_keys.count(KEY_R) || pressed_keys.count(KEY_F)){
				// Fire ray from camera and build a widget mesh from it.
				if(pressed_keys.count(KEY_F)){
					// F clears the last trace. visualizePaths appends by default.
					world_state->addWidgetData("RayWidgets", {}, false);
				}
				
				FVec3 ray_dir = camera.basis.v1 + randomComponents(random) * 0.05;
				Ray camera_ray = {camera.pos, ray_dir.normal()};
				raytracer.visualizePaths(m_simcache, {camera_ray});
				renderer_should_update = true;
			}else if(pressed_keys.count(KEY_SPACE)){
				// Clear all currently existing widgets
				world_state->addWidgetData("RayWidgets", {}, false);
				renderer_should_update = true;
			}

			if(pressed_keys.count(KEY_PLUS_SIGN) || pressed_keys.count(KEY_MINUS_SIGN)){
				int scale_modifier = 10;
				if(pressed_keys.count(KEY_MINUS_SIGN)){
					scale_modifier = -10;
				}

				IVec2 scale_vec = {scale_modifier, scale_modifier};
				image_config.num_pixels = image_config.num_pixels += scale_vec;
				image_config.num_pixels = max(image_config.num_pixels, {0, 0});
				printf("Adjusted resolution to (%iw x %ih pixels)\n", 
					image_config.num_pixels.x, 
					image_config.num_pixels.y);
			}else if(pressed_keys.count(KEY_0)){
				should_raytrace = !should_raytrace;
				if(should_raytrace){
					printf("Now in \"Raytrace Mode\" (expensive)\n");
				}else{
					printf("Now in \"Preview Mode\" (cheap)\n");
				}
				printf("\tReminder, press 'c' to capture an image\n");
			}

			// Only check for "heavy" functions at intervals to prevent rapid retriggering.
			if(debounce_time > debounce_window){
				debounce_time = 0;
				if(pressed_keys.count(KEY_P)){
					// Reload all shaders and settings from their respective files
					SystemInstruction render_instruction = {
						.type=INSTRUCTION_GENERAL_TEXT,
						.text_instruction={RENDERER_RELOAD_SHADERS_FROM_FILE}
					};
					m_renderer.sendInstruction(render_instruction);
					
					loadSettingsFromFile("SETTINGS.txt");

					render_settings = initRenderSettings(m_settings_ptr);
					image_config = render_settings.image_config;
					auto rns = m_settings_ptr->namespaceRef("RAYTRACING");
					raytracer.m_should_compress_failed_paths = rns["ShouldCompressFailedPaths"].val_bool;
				}

				if(pressed_keys.count(KEY_L)){
					// Toggle normal and wireframe rendering
					rendermode_index = !rendermode_index;
					SystemInstruction render_instruction = {
						.type=INSTRUCTION_GENERAL_TEXT,
						.text_instruction={FACE_RENDERING_OPTIONS[rendermode_index]}
					};
					m_renderer.sendInstruction(render_instruction);
				}

				if(pressed_keys.count(KEY_I)){
					// Toggle widget rendering
					widget_render_index = !widget_render_index;
					SystemInstruction render_instruction = {
						.type=INSTRUCTION_GENERAL_TEXT,
						.text_instruction={WIDGET_RENDERING_OPTIONS[widget_render_index]}
					};
					m_renderer.sendInstruction(render_instruction);
				}

				if(pressed_keys.count(KEY_C)){
					// Raytrace
					if(should_raytrace){
						printf("Raytracing image (this will take a while)...\n");
						raytracer.renderImage(m_simcache, camera, render_settings);
					}else{
						printf("Previewing image (this might take a while)...\n");
						raytracer.renderPreview(m_simcache, camera, render_settings.image_config);
					}
					renderer_should_update = true;
				}

				if(pressed_keys.count(KEY_G)){
					// Prints the player's current position
					printf("Current Player Position: "); printPODStruct(player.position); printf("\n");
				}

				if(pressed_keys.count(KEY_X) || pressed_keys.count(KEY_O)){
					// Place portal exit(X) or opening(O)
					int index_to_move = pressed_keys.count(KEY_X);
					Portal& portal = m_simcache.m_reference_world->m_temp_portal;
					portal.locations[index_to_move] = player.position;

					std::vector<Widget> portal_widgets = Debug::portalWidgets(portal);
					world_state->addWidgetData("PortalWidgets", portal_widgets, false);
					renderer_should_update = true;
				}
			}

			if(renderer_should_update){
				updateWidgetAssets(m_renderer);	
			}
		}

		// Simulate world dynamics. Ai, Physics, etc.
		if(should_execute_simulation){
			// TODO: Add simulation code	
		}
		
		// Render world state from player perspective
		if(m_window_ptr){
			m_renderer.render(m_simcache, camera);
			m_window_ptr->swapBuffers();
		}

		// End of frame. Wait until next frame.
		++num_cycles;
		debounce_time += target_frametime;
		pause(target_frametime);
	}
	m_window_ptr->terminateWindow();

	printf("Main loop terminated. Completed %li cycles.\n", num_cycles);
}

std::vector<InputEvent> VG::Engine::getInputEvents(){
	/*
	Gathers input from the GLFW window and returns a list of InputEvent structs
	*/

	std::vector<InputEvent> events;

	if(m_window_ptr){
		//-------------------------------------------------------
		// Keyboard inputs here
		//-------------------------------------------------------
		// YAGNI: More efficient query method
		KeyEventType queried_key_types[] = {
			KeyEventType::KEY_PRESSED,
			KeyEventType::KEY_HELD,

			// Seems to fire for any non-pressed key
			//KeyEventType::KEY_RELEASED  
		};
		for(KeyEventType keytype : queried_key_types){
			// 65 - 90 is ASCII range
			// NOTE: 31 and below are invalid
			for(int keycode = 32; keycode <= 348; ++keycode){ 
				if(m_window_ptr -> isKeyInState(keytype, keycode)){
					InputEvent new_event;
					new_event.source_type = InputSourceType::DEVICE_KEYBOARD;
					new_event.keyboard.event_type = keytype;
					new_event.keyboard.keycode = keycode;

					events.push_back(new_event);
				}
			}
		}

		//-------------------------------------------------------
		// Mouse inputs here
		//-------------------------------------------------------
		// TODO: Actual mouse inputs

		//-------------------------------------------------------
		// Controller type 1 inputs here
		//-------------------------------------------------------
		// TODO: Actual controller inputs
	}

	// TODO: Gather network inputs here


	// Catch configuration issues before they go any further
	for(InputEvent event : events){
		assert(event.source_type != InputSourceType::DEVICE_INVALID);
		assert(event.source_type != InputSourceType::DEVICE_UNRECOGNIZED);
	}

	return events;
}

Settings VG::Engine::defaultSettings(){
	/*
	Static method to generate a settings object for any values not defined
	in the settings file.
	*/

	Settings settings;

	Settings::Namespace engine;
	engine["TargetFPS"] = 30;
	engine["NumWorkerThreads"] = 1;
	engine["ShouldRenderWidgets"] = true;
	engine["UseGraphics"] = true;
	engine.contained_namespaces = {"WINDOW", "INPUT", "WORLD", "RAYTRACING", "ACCELERATION"};
	settings.update("ENGINE", engine);

	Settings::Namespace window;
	window["ShouldStartFullscreen"] = true;
	settings.update("WINDOW", window);

	Settings::Namespace input;
	input["KeyDebounceTimeSeconds"] = 0.1f;
	settings.update("INPUT", input);

	Settings::Namespace world;
	world["SkyColor"] = FVec3{0, 0, 0};
	world.contained_namespaces = {"CHUNK_GEN"};
	settings.update("WORLD", world);

	Settings::Namespace chunk_gen;
	chunk_gen["Seed"] = 0;
	chunk_gen["GenerationAlgorithm"] = "Default";
	chunk_gen["MinBounds"] = IVec3{-1, -1, -1};
	chunk_gen["MaxBounds"] = IVec3{0, 0, 0};
	settings.update("CHUNK_GEN", chunk_gen);

	Settings::Namespace raytracing;
	raytracing["ImageDimensions"] = IVec2{500, 500};
	raytracing["TileDimensions"] = IVec2{32, 32};
	raytracing["NumRenderThreads"] = 1;
	raytracing["RaysPerPixel"] = 1;
	raytracing["ShouldCompressFailedPaths"] = false;
	raytracing["SkyColor"] = FVec3{0.5294, 0.8078, 0.9216};
	raytracing["SunColor"] = FVec3{0.9980, 0.8314, 0.2510};
	raytracing["SunDirection"] = FVec3{-0.2, -0.2, -3.0}.normal();
	raytracing["SkyBrightnessMultiplier"] = 1.0f;
	raytracing["SunBrightnessMultiplier"] = 5.0f;
	settings.update("RAYTRACING", raytracing);

	Settings::Namespace acceleration;
	acceleration.contained_namespaces = {"VKDTREE", "MKDTREE", "SVO"};
	settings.update("ACCELERATION", acceleration);

	Settings::Namespace vkdtree;
	vkdtree["MaxDepth"] = 20;
	vkdtree["MandatoryLeafVolume"] = 8;
	settings.update("VKDTREE", vkdtree);

	return settings;
}
