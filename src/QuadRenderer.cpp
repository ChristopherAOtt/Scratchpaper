#include "QuadRenderer.hpp"

QuadRenderer::QuadRenderer(){
	m_shader_ptr = std::unique_ptr<QuadShader>(new QuadShader);
	m_viewport_dims = {10, 10};
	m_use_texture_dims = true;

	initQuadData();
	initShader("Shaders/QuadShader.shader");
}

QuadRenderer::~QuadRenderer(){
	
}

void QuadRenderer::initShader(std::string filepath){

	// TODO: Move the definition from here and the renderer into the BaseShader
	// class.
	const std::string VERTEX_SECTION_NAME = "VERTEX_SHADER";
	const std::string FRAGMENT_SECTION_NAME = "FRAGMENT_SHADER";

	m_shader_ptr->destroy();

	auto shader_map = loadShadersFromFile(filepath);
	for(auto name : {VERTEX_SECTION_NAME, FRAGMENT_SECTION_NAME}){
		auto iter = shader_map.find(name);
		assert(iter != shader_map.end());
	}

	m_shader_ptr->compileShaders(
		shader_map[VERTEX_SECTION_NAME], 
		shader_map[FRAGMENT_SECTION_NAME]);

	if(!m_shader_ptr->isValid()){
		printf("ERROR: Shader compilation failed!\n");
	}
}

void QuadRenderer::setViewportDims(IVec2 dimensions){
	m_viewport_dims = dimensions;
}

void QuadRenderer::render(const Framebuffer& buffer){
	RenderData data = {
		.dims=buffer.getDimensions(),
		.fbo_id=buffer.getFramebufferId(),
		.color_id=buffer.getColorbufferId(),
		.depth_id=buffer.getDepthbufferId(),
	};

	render(data);
}

void QuadRenderer::render(Uint32 texture_id, IVec2 dimensions){
	/*
	REFACTOR: Pass in a ResourceManager reference and ResourceHandle
		instead of the raw data. 
	*/

	RenderData data = {
		.dims=dimensions,
		.fbo_id=0,
		.color_id=texture_id,
		.depth_id=0,
	};

	render(data);
}

void QuadRenderer::render(RenderData data){
	/*
	Generic quad rendering function
	*/

	assert(m_shader_ptr->isValid());

	IVec2 render_dims = m_viewport_dims;
	if(m_use_texture_dims){
		render_dims = data.dims;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, data.fbo_id);
	//glViewport(0, 0, render_dims.x, render_dims.y);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE); // Don't update depth buffer for these shaders
	{
		m_shader_ptr->use();
		m_shader_ptr->shouldFlipVertical(true);
		glBindVertexArray(m_quad_vao_id);

		// Bind uniform samplers, then load the textures
		glUniform1i(m_shader_ptr->colorLocation(), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, data.color_id);

		Uint32 depth_buffer_id = data.depth_id;
		if(data.depth_id != 0){
			//glUniform1i(m_shader_ptr->depthLocation(), 1);
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, depth_buffer_id);
		}

		glDrawArrays(
			GL_TRIANGLES, // Mode
			0,            // Index of fist element
			3 * 2         // Number of elements to draw
		);

		m_shader_ptr->stop();
	}
	
	glDepthMask(GL_TRUE); // Don't block depth updates for other renders
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
}

void QuadRenderer::initQuadData(){
	/*
	Creates a quad from two triangles and submits the data to the API
	*/

	struct QuadVertex{
		FVec3 screenspace_pos;
		FVec2 uv;
	};
	static_assert(sizeof(QuadVertex) == 5 * sizeof(float));

	constexpr QuadVertex quad_vertices[] = {
		// Triangle 1
		{{ 1.0, -1.0, 0.0},	{1.0, 0.0}},
		{{ 1.0,  1.0, 0.0},	{1.0, 1.0}},
		{{-1.0,  1.0, 0.0},	{0.0, 1.0}},

		// Triangle 2
		{{ 1.0, -1.0, 0.0},	{1.0, 0.0}},
		{{-1.0,  1.0, 0.0},	{0.0, 1.0}},
		{{-1.0, -1.0, 0.0},	{0.0, 0.0}}
	};

	Uint32 vao_id, vbo_id;
	glGenVertexArrays(1, &vao_id);
	glGenBuffers(1, &vbo_id);
	glBindVertexArray(vao_id);
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * 6, quad_vertices, 
			GL_STATIC_DRAW);

		// Positions
		glEnableVertexAttribArray(0);	
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), 
			(void*) (0));
		
		// UV's
		glEnableVertexAttribArray(1);	
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), 
			(void*) (sizeof(float) * 3));
	}
	glBindVertexArray(0);

	m_quad_vao_id = vao_id;
}
