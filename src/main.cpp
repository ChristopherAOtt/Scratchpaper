#include <string>
#include <stdio.h>
#include <stdlib.h>

#include "Engine.hpp"
#include "WorldState.hpp"
#include "RayTracing.hpp"
#include "Renderer.hpp"
#include "WidgetShader.hpp"
#include "ChunkManager.hpp"
#include "MeshGenerator.hpp"
#include "FileIO.hpp"
#include "ResourceManager.hpp"
#include "Settings.hpp"
#include "KDTree.hpp"
#include "QOI.hpp"
#include "Image.hpp"
#include "Debug.hpp"
#include "MeshLoader.hpp"

float randFloat(){
	return (float)rand() / (float)RAND_MAX;
}

WorldState* initWorldState(){
	/*
	Heap-allocates a trivial WorldState object for use by the engine.
	*/
	WorldState* world_ptr = new WorldState();
	world_ptr->m_name = "Default World State";
	world_ptr->m_fog_color = {0.0f, 0.0f, 0.0f};
	world_ptr->m_temp_portal = {
		.radius=10.0f,
		.locations={
			{-50,-50,-50},
			{50, 50, 50}
		}
	};

	FVec2 dummy_uv = {0, 0};
	
	// Basis vectors at the center of the world.
	std::vector<Widget> default_widgets;
	for(int i = 0; i < 3; ++i){
		FVec3 axis_color = {0, 0, 0};
		FVec3 axis_end_pos = {0, 0, 0};
		axis_color[i] = 1.0;
		axis_end_pos[i] = 100.0;

		Widget axis_widget = {
			WIDGET_LINE,
			.line={
				{{0, 0, 0},    dummy_uv, axis_color},
				{axis_end_pos, dummy_uv, axis_color}
			}
		};

		default_widgets.push_back(axis_widget);
	}

	// Add some scratch visualizations
	if(false){
		RandomGen gen;
		gen.splitmix = {1234};
		for(int i = 0; i < 30000; ++i){
			FVec3 normal = randomUnitNormal(gen);

			Widget normal_widget = {
				WIDGET_MARKER,
				.marker={
					{normal * 300, dummy_uv, {1,1,1}}
				}
			};
			default_widgets.push_back(normal_widget);
		}
	}

	world_ptr->addWidgetData("Landmarks", default_widgets, false);

	return world_ptr;
}

void runEngineMainLoop(int argc, char** argv){
	/*
	Inits the engine object and starts the loop.

	TODO: Actually use command line args at some point
	*/

	VG::Engine engine;
	//engine.setDefaultSettings(VG::Engine::defaultSettings());
	engine.loadSettingsFromFile("SETTINGS.txt");
	engine.setTargetWorld(initWorldState());
	engine.initTargetWorld();
	engine.initWindow();
	engine.runMainLoop();
}

std::vector<int> subset(std::vector<int> vec, int start, int end){
	std::vector<int> result;
	for(int i = start; i < end; ++i){
		result.push_back(vec[i]);
	}
	return result;
}

void printVector(std::vector<int> vec){
	printf("Vector (len %li){", vec.size());
	for(int i : vec){
		printf("%i, ", i);
	}
	printf("}\n");
}


void runSplitterRecursive(std::vector<int>& density_vec, int depth, int& total_iterations){
	printf("\n\n\n\n----------------------------------------------------\n");
	++total_iterations;
	printf("RunSplitterRecursive. Depth %i\n", depth);
	printVector(density_vec);
	int vec_size = density_vec.size();
	if(vec_size < 4){
		printf("Leaf.\n");
		printf("----------------------------------------------------\n");
		return;
	}

	int best_run_len = 0;
	int best_run_start_index = 0;

	int curr_run_start_index = 0;
	int curr_run_len = 0;
	int curr_run_density = density_vec[0];
	for(int i = 0; i < vec_size; ++i){
		if(density_vec[i] != curr_run_density){
			printf("Run terminated @ index %i!\n", i);
			// Run terminated
			if(curr_run_len > best_run_len){
				printf("\tLength %i better than prior of %i\n", curr_run_len, best_run_len);
				best_run_start_index = curr_run_start_index;
				best_run_len = curr_run_len;
			}

			curr_run_len = 1;
			curr_run_start_index = i;
			curr_run_density = density_vec[i];
		}else{
			// Run continues
			++curr_run_len;
		}
	}

	// Force one last check
	if(curr_run_len > best_run_len){
		printf("\tLength %i better than prior of %i\n", curr_run_len, best_run_len);
		best_run_start_index = curr_run_start_index;
		best_run_len = curr_run_len;
	}

	printf("Best run @%i with %i items\n", best_run_start_index, best_run_len);

	if(best_run_len == vec_size){
		printf("\tLEAF.\n");
		printf("----------------------------------------------------\n");
		return;
	}

	int best_split_pos = best_run_start_index;
	if(best_run_start_index == 0){
		best_split_pos = best_run_len;
		printf("Best run at start; need to fix index! Now at %i\n", best_run_len);
	}

	float ratio = (float)best_split_pos / (float)density_vec.size();
	float threshold = 0.1;
	if(ratio < threshold || best_run_len < 2){
		printf("Partition ratio %f\n", ratio);
		printf("This run sucked! Splitting in half!\n");
		best_split_pos = density_vec.size() / 2;
	}

	int final_result = min(max(best_split_pos, 1), density_vec.size() - 1);
	printf("Best split: %i. Final result %i\n", best_split_pos, final_result);
	
	auto v1 = subset(density_vec, 0, final_result);
	auto v2 = subset(density_vec, final_result, density_vec.size());
	printf("A: "); printVector(v1);
	printf("B: "); printVector(v2);
	printf("----------------------------------------------------\n");

	runSplitterRecursive(v1, depth + 1, total_iterations);
	runSplitterRecursive(v2, depth + 1, total_iterations);
}

void testSplitterStrategies(){
	std::vector<int> densities = {
		0, 0, 0, 0,
		1, 2, 3, 9, 9, 9, 9, 9, 8, 5, 2,
		0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
		1, 2, 9, 5, 7, 1,
		0, 0, 0, 0,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  // 10
		1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 2, 1,
		0, 0, 0, 9, 8, 7, 6, 5, 4, 3, 3, 3, 2, 1, 0, 0, 0
	};

	int total_iterations = 0;
	auto subvector = subset(densities, 0, densities.size());
	runSplitterRecursive(subvector, 0, total_iterations);
	printf("Total iterations: %i\n", total_iterations);
}

void printBits(Uint64 data){
	Uint64 BITFLAG = 0x00000000'00000001;

	constexpr int num_bits = sizeof(Uint64) * BITS_PER_BYTE;
	int bits[num_bits];
	for(int i = 0; i < num_bits; ++i){
		int result = (data >> i) & BITFLAG;
		bits[i] = result;
	}
	printf("{");
	for(int i = num_bits - 1; i >=0; --i){
		printf("%i", bits[i]);
	}
	printf("}");
}

void bitPatterns(std::vector<Uint64> values, std::string label){
	Uint64 on_test  = 0xffffffff'ffffffff;
	Uint64 off_test = 0x00000000'00000000;
	for(auto value : values){
		on_test &= value;
		off_test |= value;
	}
	off_test = ~off_test;

	printf("Info for '%s'\n", label.c_str());
	printf("\tAlways On : "); printBits(on_test); printf("\n");
	printf("\tAlways Off: "); printBits(off_test); printf("\n");
}

void scratchDictInterface(){
	/*
	Get a feel for how reasonable it feels to use the interface.
	*/

	PODDict dict = PODDict::init();
	dict.update("Four", 4);
	dict.update("Pi", 3.141592f);
	dict.update("To be or not to be", false);
	dict.update("InitialBounds", IVec3{-1, -1, -1});
	dict.update("FinalBounds",   IVec3{1, 1, 1});
	dict.update("DroneVelocity", FVec3{0, 0, -9.81});

	for(auto iter = dict.begin(); iter != dict.end(); ++iter){
		printPODStruct(iter.first()); printf(":"); printPODStruct(iter.second()); printf("\n");
	}

	for(auto [key, value] : dict){
		printPODStruct(key); printf(":"); printPODStruct(value); printf("\n");
	}

	std::string search_key = "FinalBounds";
	printf("Key for '%s' @%i\n", search_key.c_str(), dict.keyIndex(search_key.c_str()));
}

void scratchDeclarationLoading(){
	std::string filepath = "Resources.txt";
	auto declarations = FileIO::loadResourceDeclarations(filepath);
	for(FileIO::ResourceDeclaration& decl : declarations){
		printf("Declared [%s] @%s\n", decl.name.c_str(), decl.source_filepath.c_str());
	}
}

void countBits(std::vector<Uint64> values){
	constexpr int NUM_BITS = sizeof(Uint64) * BITS_PER_BYTE;
	int bit_counts[NUM_BITS] = {};
	for(Uint64 value : values){
		for(Uint64 i = 0; i < NUM_BITS; ++i){
			bit_counts[i] += ((value & (1ULL << i)) != 0);
		}
	}

	printf("Bit Counts for %li values:\n", values.size());
	for(int i = 0; i < NUM_BITS; ++i){
		printf("[%i]: %i\n", i, bit_counts[i]);
	}
};

void testMeshLoad(int argc, char** argv){
	std::string filepath = "res/TestObjs/Hard/FlorenceBridge/PonteVecchio.obj";
	if(argc > 1){
		filepath = std::string(argv[1]);
	}

	printf("Loading from file '%s'\n", filepath.c_str());
	auto [is_valid, mesh] = MeshLoader::Obj::loadFromFile(filepath);
	if(is_valid){
		printf("Output was valid\n");
	}else{
		printf("Output was invalid\n");
	}
}

void scratchFloatMatcher(){
	struct Test{
		const char* text;
		bool is_malformed;
		bool is_int;
		double expected_double_value;
		int expected_int_value;
	};

	Test test_inputs[] = {
		{"1.2",       false, false, 1.2},
		{"3.141592",  false, false, 3.141592},
		{"1.7e-5",    false, false, 1.7e-5},
		{"0.18e251",  false, false, 0.18e251},
		{"0.9001e+4", false, false, 0.9001e+4},
		{"0.9001e-4", false, false, 0.9001e-4},
		{"3e5",       false, false, 3e5},
		{"3e-5",      false, false, 3e-5},

		{"12345",     false, true,  0, 12345},
		{"0",         false, true,  0, 0},
		{"-7",        false, true,  0, -7},
		{"7",         false, true,  0, 7},

		{"3.1415e",   true},
		{"3e",        true},
	};

	int RED = 31;
	int GREEN = 32;
	int RESET = 0;

	int RESULT_BOOL_COLORS[] = {RED, GREEN};

	for(Test test : test_inputs){
		printf("[%s|", test.text);
		if(test.is_malformed){
			printf("MALFORMED INPUT]-->");
		}else{
			if(test.is_int){
				printf("INT]-->");
			}else{
				printf("FLOAT]-->");
			}
		}
		printf("\t\t");

		InputBuffer buffer{std::string(test.text)};
		Tokenizer tokenizer(&buffer);
		auto match_result = tokenizer.attemptNumeric();
		
		if(test.is_malformed && match_result.result == RESULT_SUCCESS){
			printf("\e[1;%im", RED);
		}

		printf("Match[%s, ", MATCH_RESULT_TYPE_STRINGS[match_result.result]);
		if(!test.is_malformed){
			if(test.is_int){
				int int_result = match_result.success.token.data.value_int;
				int int_expected = test.expected_int_value;
				
				printf("\e[1;%im", RESULT_BOOL_COLORS[int_result == int_expected]);
				printf("Ref %i vs Test %i]\n", int_expected, int_result);
			}else{
				double d_result = match_result.success.token.data.value_real;
				double d_expected = test.expected_double_value;
				
				printf("\e[1;%im", RESULT_BOOL_COLORS[d_result == d_expected]);
				printf("Ref %06f vs Test %06f]\n", d_expected, d_result);
			}
		}else{
			printf("]\n");
		}
		printf("\e[1;%im", RESET);
	}
}

#include <mutex>
void threadMembersTest(int argc, char** argv){
	class FakeClass{
		private:
			Int64 m_data{0};
			std::mutex add_mutex;

		private:
			void addValue(Int64 value){
				/*
				Nasty function meant to slow down a simple addition enough
				for data races to start happening
				*/

				if(m_use_mutex){
					add_mutex.lock();
				}
				
				if(m_should_print){
					printf("Adding %li: ", value);	
				}
				
				sleep((rand() % 1000) / 1000.0f);
				m_data += value;

				if(m_should_print){
					printf("Done! Value now %li\n", m_data);
				}

				if(m_use_mutex){
					add_mutex.unlock();
				}
			}

		public:
			bool m_use_mutex{true};
			bool m_should_print{true};
			void runAdder(Int64 num_iters){
				constexpr Int64 BLOCK_SIZE = 17;
				std::thread* thread_arr = (std::thread*) malloc(BLOCK_SIZE * sizeof(std::thread));
				Int64 i = 0;
				while(i < num_iters){
					Int64 num_to_run = min(BLOCK_SIZE, num_iters - i);
					if(m_should_print){
						printf("New block. Running %li threads.\n", num_to_run);
					}

					// Launch them all
					for(Int64 r = 0; r < num_to_run; ++r){
						thread_arr[r] = std::thread(&FakeClass::addValue, this, ++i);
					}

					// Join them all
					for(Int64 r = 0; r < num_to_run; ++r){
						thread_arr[r].join();
					}
				}
				free(thread_arr);

				Int64 correct_sum = (num_iters * (num_iters + 1)) / 2;
				printf("\n\n\nAll runs complete\n");
				printf("Final Sum:   %li\n", m_data);
				printf("Correct sum: %li\n", correct_sum);

				float f_correct = (float) correct_sum;
				float percent_error = abs(m_data - f_correct) / f_correct * 100;
				printf("%%Error %f\n", percent_error);
			}
	};

	int num_iters = 100;
	bool use_mutex = true;
	bool should_print = true;
	if(argc > 1){
		num_iters = atoi(argv[1]);
	}
	if(argc > 2){
		use_mutex = atoi(argv[2]);
	}
	if(argc > 3){
		should_print = atoi(argv[3]);
	}

	FakeClass fake;
	fake.m_use_mutex = use_mutex;
	fake.m_should_print = should_print;
	fake.runAdder(num_iters);
}

int main(int argc, char** argv){
	//scratchDeclarationLoading();
	//testMeshLoad(argc, argv);
	//scratchFloatMatcher();
	
	//testSphereIntersect();
	runEngineMainLoop(argc, argv);

	return 0;
}
