#include "MeshingOpenGL.hpp"

//-------------------------------------------------------------------------------------------------
// Vertex description functionality
//-------------------------------------------------------------------------------------------------
VertexDescription vertexDescriptionVoxelVertex(){
	VertexDescription description;
	
	description.num_described_members = 3;
	description.vertex_total_size = sizeof(VoxelMeshVertex);
	description.member_descriptions[0] = {TYPE_FLOAT, 3};
	description.member_descriptions[1] = {TYPE_FLOAT, 2};
	description.member_descriptions[2] = {TYPE_UINT, 1};

	return description;
}

VertexDescription vertexDescriptionVoxelLightingVertex(){
	VertexDescription description;
	
	description.num_described_members = 4;
	description.vertex_total_size = sizeof(LightmappedVoxelMeshVertex);
	description.member_descriptions[0] = {TYPE_FLOAT, 3};
	description.member_descriptions[1] = {TYPE_FLOAT, 2};
	description.member_descriptions[2] = {TYPE_UINT, 1};
	description.member_descriptions[3] = {TYPE_FLOAT, 3};

	return description;
}

VertexDescription vertexDescriptionWidgetVertex(){
	VertexDescription description;
	
	description.num_described_members = 3;
	description.vertex_total_size = sizeof(WidgetMeshVertex);
	description.member_descriptions[0] = {TYPE_FLOAT, 3};
	description.member_descriptions[1] = {TYPE_FLOAT, 2};
	description.member_descriptions[2] = {TYPE_FLOAT, 3};

	return description;
}

std::vector<Bytes1> toByteVector(void* arr_base_ptr, int num_bytes){
	/*
	Used to convert multiple different Vertex vectors to a consistent
	byte format
	*/

	std::vector<Bytes1> out_bytes;
	out_bytes.reserve(num_bytes);
	
	Bytes1* arr_ptr = (Bytes1*) arr_base_ptr;
	for(int i = 0; i < num_bytes; ++i){
		out_bytes.push_back(arr_ptr[i]);
	}

	return out_bytes;
}


//-------------------------------------------------------------------------------------------------
// Mesh generation functionality
//-------------------------------------------------------------------------------------------------
WidgetMeshOpengl MesherOpenGL::toOpenglFormat(const WidgetMesh& mesh){
	/*
	Currently just a pass-through function. If the format of the WidgetMesh ever changes from
	the OpenGL format then the conversion code will go here.
	*/
	return mesh;
}

ChunkVoxelMeshOpengl MesherOpenGL::toOpenglFormat(const ChunkVoxelMesh& mesh){
	/*
	Currently just a pass-through function. If the format of the ChunkVoxelMesh ever changes from
	the OpenGL format then the conversion code will go here.
	*/
	
	return mesh;
}

RigidTriangleMeshOpengl MesherOpenGL::toOpenglFormat(const TriangleMesh& mesh){
	/*
	Reorganizes the triangles into triplets of vertices and sorts so that
	all triangles of a given material are in a contiguous range.
	
	WARNING: This doesn't transfer triangles without a material.
	*/

	RigidTriangleMeshOpengl output_mesh;
	output_mesh.vertices.reserve(mesh.triangles.size() * NUM_VERTICES_PER_TRIANGLE);
	output_mesh.indices.reserve(mesh.triangles.size() * NUM_VERTICES_PER_TRIANGLE);

	std::unordered_map<std::string, std::vector<Range32>> ranges_by_material;
	for(auto const& named_range : mesh.material_ranges){
		ranges_by_material[named_range.name].push_back(named_range.range);
	}

	Int32 tri_count = 0;
	Int32 curr_index = 0;
	std::unordered_map<std::string, Range32> output_material_ranges;
	for(auto const& [name, input_range_list] : ranges_by_material){
		Range32 output_range = {tri_count, };
		for(auto const& range : input_range_list){
			for(Int32 t = range.origin; t < range.origin + range.extent; ++t){
				const TriangleMesh::Triangle& triangle = mesh.triangles[t];
				for(int i = 0; i < NUM_VERTICES_PER_TRIANGLE; ++i){
					RigidTriangleMeshVertex new_vertex = {
						.pos=triangle.positions[i],
						.uv=triangle.uvs[i],
						.normal=triangle.normals[i],
					};
					output_mesh.vertices.push_back(new_vertex);
					output_mesh.indices.push_back(curr_index++);
				}
			}
			tri_count += range.extent;
		}

		output_range.extent = tri_count - output_range.origin;
		output_material_ranges[name] = output_range;
	}
	output_mesh.material_ranges = output_material_ranges;

	return output_mesh;
}
