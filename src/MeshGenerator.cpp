#include "MeshGenerator.hpp"

//-------------------------------------------------------------------------------------------------
// Local helper functions
//-------------------------------------------------------------------------------------------------
bool outOfBounds(IVec3 coords){
	for(int i = 0; i < 3; ++i){
		if(coords[i] < 0 || coords[i] >= CHUNK_LEN){
			return true;
		}
	}
	return false;
}

bool isSolid(VoxelType type){
	/*
	Returns true if the type is a solid voxel
	Purpose: True if the voxel should emit a face
	*/

	return type != VoxelType::Air && type != VoxelType::EMPTY;
}

bool isOpaque(VoxelType type){
	/*
	Returns true if the voxel is opaque.
	Purpose: True if the voxel edges facing this type should not emit a face.
	TODO: Add support for glass and other transparent types.
	*/

	return type != VoxelType::Air && type != VoxelType::EMPTY;	
}

IVec3 neighborCoords(IVec3 coords, int face_index){
	/*
	Given a set of coordinates and a face index (0 - 5), return the 
	coordinates of the neighbor in that direction.
	*/

	IVec3 neighbor_coords = coords;
	neighbor_coords[face_index / 2] += SIGNED_AXIS_OFFSETS[face_index % 2];
	return neighbor_coords;
}


MeshFace squareFace(IVec3 coords, int face_index){
	/*
	Given a set of coordinates and a face index (0 - 5), return the vertex
	positions of the face in that direction
	*/
	
	MeshFace output_face;
	const int* vert_index_arr = VERT_INDICES[face_index];
	for(int i = 0; i < NUM_VERTICES_PER_FACE; ++i){
		int vert_to_get = vert_index_arr[i];
		FVec3 vert_position = CUBE_VERTEX_POSITIONS[vert_to_get];
		output_face.vertex_positions[i] = vert_position + toFloatVector(coords);
	}
	
	return output_face;
}


//-------------------------------------------------------------------------------------------------
// Meshing functions
//-------------------------------------------------------------------------------------------------



MesherAdjacencyGrid generateAdjacencyGrid(const ChunkTable* table, CellAddress addr){
	/*
	Wrapper function for generateAdjacencyGrid that performs the chunk
	lookup given an address.

	NOTE: This should never be called on a non-existant chunk. 
	*/

	// Get the chunk data
	assert(table != NULL);
	const RawVoxelChunk* chunk_ptr = table->getChunkPtr(addr.corner_addr);
	assert(chunk_ptr != NULL);
	RawVoxelChunk chunk = *chunk_ptr;	

	return generateAdjacencyGrid(chunk);
}

MesherAdjacencyGrid generateAdjacencyGrid(const RawVoxelChunk& chunk_data){
	/*
	Generates a grid of AdjacencyNode structs corresponding to each voxel in the
	target chunk.
	*/	

	MesherAdjacencyGrid grid;
	int num_total_faces = 0;

	// Determine for each face of each voxel whether or not a face should be emitted
	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		VoxelType current_type = chunk_data.voxelTypeAt({x, y, z});
		AdjacencyNode new_node = {.voxel_type=current_type};
		new_node.flags.all_bits = new_node.flags.all_bits & FLAG_CLEAR;  // Start all bits at 0

		if(isSolid(current_type)){
			for(int f = 0; f < NUM_FACES_PER_VOXEL; ++f){
				IVec3 neighbor_coords = neighborCoords({x, y, z}, f);

				// Get neighbor type, assume air if unavailable
				// TODO: Actually reference neighboring chunk values
				VoxelType neighbor_type = VoxelType::Air;
				if(!outOfBounds(neighbor_coords)){
					neighbor_type = chunk_data.voxelTypeAt(neighbor_coords);
				}

				// Only emit a face if the neighbor is a transparent voxel of a different type
				if(!isOpaque(neighbor_type) && (neighbor_type != current_type)){
					new_node.flags.all_bits |= FLAGS_IN_ORDER_BY_FACE[f];
					++num_total_faces;
				}
			}
		}

		grid.nodes[linearChunkIndex(x, y, z)] = new_node;
	}

	grid.num_total_faces = num_total_faces;
	return grid;
}


ChunkVoxelMesh generateChunkMesh(const MesherAdjacencyGrid& adjacency, CellAddress addr){
	/*
	Given an adjacency grid, generate a regular chunk mesh.
	*/

	ChunkVoxelMesh mesh;
	mesh.address = addr;
	mesh.vertices.reserve(adjacency.num_total_faces * NUM_VERTICES_PER_FACE);
	mesh.indices.reserve(adjacency.num_total_faces * NUM_INDICES_PER_FACE);

	MeshIndex curr_index = 0;
	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		const AdjacencyNode& curr_node = adjacency.nodes[linearChunkIndex(x, y, z)];

		if(isSolid(curr_node.voxel_type) && (curr_node.flags.all_bits & FLAG_ANY_VALUES_SET)){
			for(int f = 0; f < NUM_FACES_PER_VOXEL; ++f){
				if(curr_node.flags.all_bits & FLAGS_IN_ORDER_BY_FACE[f]){
					// Emit a face
					MeshFace face = squareFace({x, y, z}, f);

					// Emit the vertices
					for(int i = 0; i < NUM_VERTICES_PER_FACE; ++i){
						VoxelMeshVertex new_vertex = {
							.position=face.vertex_positions[i],
							.uv=DUMMY_UV,
							.type_index=curr_node.voxel_type,
							.normal_index=(Uint32)f
						};
						mesh.vertices.push_back(new_vertex);
					}

					// Emit the indices
					for(int i = 0; i < NUM_INDICES_PER_FACE; ++i){
						mesh.indices.push_back(curr_index + FACE_INDICES[i]);
					}
					curr_index += NUM_VERTICES_PER_FACE;
				}
			}
		}
	}

	return mesh;
}

ChunkVoxelMesh generateGreedyChunkMesh(const MesherAdjacencyGrid& adjacency){
	/*
	Given an adjacency grid, generate a greedy chunk mesh.
	WARNING: Placeholder implementation.
	*/
	assert(false);
	ChunkVoxelMesh mesh;

	return mesh;
}

LightmappedChunkVoxelMesh generateLightmappedChunkMesh(const MesherAdjacencyGrid& adjacency){
	/*
	Given an adjacency grid, generate a lightmapped chunk mesh.
	WARNING: Placeholder implementation.
	*/
	assert(false);

	LightmappedChunkVoxelMesh mesh;

	return mesh;
}


WidgetMesh generateAdjacencyGridWidget(const MesherAdjacencyGrid& grid){
	/*
	Generates a widget where each adjacency cell with a face recommendation is shown
	as a small colored line.
	*/

	WidgetMesh output_mesh;

	constexpr FVec3 CENTERING_OFFSET = {0.5, 0.5, 0.5};
	constexpr FVec3 CENTER_COLOR = {1, 1, 1};

	MeshIndex curr_index = 0;
	for(int z = 0; z < CHUNK_LEN; ++z)
	for(int y = 0; y < CHUNK_LEN; ++y)
	for(int x = 0; x < CHUNK_LEN; ++x){
		const AdjacencyNode& curr_node = grid.nodes[linearChunkIndex(x, y, z)];

		if(isSolid(curr_node.voxel_type) && (curr_node.flags.all_bits & FLAG_ANY_VALUES_SET)){
			FVec3 center_pos = {(float)x, (float)y, (float)z};
			WidgetMeshVertex center_vertex = {
				center_pos + CENTERING_OFFSET,
				DUMMY_UV,
				CENTER_COLOR
				//VOXEL_COLOR_BY_TYPE[curr_node.voxel_type]
			};
			output_mesh.vertices.push_back(center_vertex);

			int num_emitted_faces = 0;
			for(int f = 0; f < NUM_FACES_PER_VOXEL; ++f){
				if(curr_node.flags.all_bits & FLAGS_IN_ORDER_BY_FACE[f]){
					IVec3 neighbor_coords = neighborCoords({x, y, z}, f);
					FVec3 dir_vector = toFloatVector(neighbor_coords) - toFloatVector({x, y, z});
					dir_vector = dir_vector * 0.25;

					WidgetMeshVertex tip_vertex = {
						center_vertex.pos + dir_vector,
						DUMMY_UV,
						DIRECTION_COLORS[(f / 2) * 2]
						//VOXEL_COLOR_BY_TYPE[curr_node.voxel_type]
					};

					++num_emitted_faces;
					output_mesh.vertices.push_back(tip_vertex);
					output_mesh.indices.push_back(curr_index);
					output_mesh.indices.push_back(curr_index + num_emitted_faces);
				}
			}
			curr_index += num_emitted_faces + 1; 
		}
	}

	return output_mesh;
}


WidgetMesh generateWidgetMesh(const std::vector<Widget>& widgets){
	/*
	This condenses all the widgets in the input vector into a single
	mesh. Useful for when a large number of small-detail widgets (such as rays)
	need to be displayed at the same time.

	TODO: Allow for widgets of different types (Points + Triangles) in the same mesh.

	TODO: Fix the corner order of the cuboid widget from
		CURRENT: C1=0 C2=6
		CORRECT: C1=7 C2=1
		This will involve re-writing the different corner vertices.
	*/

	WidgetMesh mesh;
	mesh.type = MESH_LINES;
	std::vector<WidgetMeshVertex>& vertices = mesh.vertices;

	FVec2 dummy_uv = {0, 0};
	Uint32 index = 0;
	for(const Widget& widget: widgets){
		if(widget.type == WidgetType::WIDGET_MARKER){
			WidgetMarker marker = widget.marker;
			FVec3 p = marker.vertex.pos;
			FVec3 c = marker.vertex.col;

			for(int d = 0; d < 3; ++d){
				FVec3 vert1 = p;
				FVec3 vert2 = p;

				vert1[d] += 1;
				vert2[d] -= 1;

				vertices.push_back({vert1, dummy_uv, c});
				vertices.push_back({vert2, dummy_uv, c});
			}

			for(int i = 0; i < 6; ++i){
				mesh.indices.push_back(index + i);	
			}
			index += 6;
		}else if(widget.type == WidgetType::WIDGET_LINE){
			WidgetLine line = widget.line;

			vertices.push_back({line.v1.pos, dummy_uv, line.v1.col});
			vertices.push_back({line.v2.pos, dummy_uv, line.v2.col});

			mesh.indices.push_back(index);
			mesh.indices.push_back(index + 1);
			index += 2;
		}else if(widget.type == WidgetType::WIDGET_CUBOID){
			/*
			Cuboids render a group of lines for every side and diagonal 
			crossbars for each face so that the edges of large cuboids are
			more visible.

			Vertex order.
			|BOTTOM|TOP|
			|2 3   |6 7|
			|0 1   |4 5|
			*/

			WidgetCuboid cuboid = widget.cuboid;
			
			// Pushing all vertices
			for(int i = 0; i < NUM_VERTICES_PER_VOXEL; ++i){
				FVec3 unit_vertex = CUBE_VERTEX_POSITIONS[i];
				WidgetMeshVertex new_vertex = {
					cuboid.origin + hadamard(unit_vertex, cuboid.extent), 
					dummy_uv, 
					cuboid.color
				};
				mesh.vertices.push_back(new_vertex);
			}

			// Pushing all indices
			constexpr int NUM_CUBOID_INDICES = 48;
			constexpr int CUBOID_INDICES[] = {
				// Bottom Plane Edges
				0, 1, 1, 3, 3, 2, 2, 0,

				// Top Plane Edges
				4, 5, 5, 7, 7, 6, 6, 4,

				// Vertical Edges
				0, 4, 1, 5, 3, 7, 2, 6,

				// Crossbars
				1, 7, 3, 5,   2, 4, 0, 6,  // +X, -X
				3, 6, 2, 7,   0, 5, 1, 4,  // +Y, -Y
				4, 7, 5, 6,   1, 2, 0, 3,  // +Z, -Z
			};
			for(int i = 0; i < NUM_CUBOID_INDICES; ++i){
				mesh.indices.push_back(index + CUBOID_INDICES[i]);
			}
			index += NUM_VERTICES_PER_VOXEL;
		}else if(widget.type == WidgetType::WIDGET_SPHERE){
			/*
			TODO: UV sphere or figure out vertex order. Fibonacci spheres looks like crap.
			*/

			auto& sphere = widget.sphere;
			
			// Handle vertices
			constexpr int NUM_SPHERE_VERTICES = 100;
			auto sphere_dirs = Geometry::fibonacciSphere(NUM_SPHERE_VERTICES);
			for(int i = 0; i < NUM_SPHERE_VERTICES; ++i){
				WidgetMeshVertex new_vertex = {
					sphere.origin + sphere_dirs[i] * sphere.radius,
					dummy_uv,
					sphere.color
				};
				mesh.vertices.push_back(new_vertex);
			}

			// Handle indices
			for(int i = 0; i < NUM_SPHERE_VERTICES - 1; ++i){
				mesh.indices.push_back(index + i);
				mesh.indices.push_back(index + i + 1);
			}
			index += NUM_SPHERE_VERTICES;
		}
	}
	
	return mesh;
}

