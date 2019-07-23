#include "client.hpp"

const char windowTitle[] = "OpenGL Test 2";

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	std::cout << "Window resized to " << width << "×" << height << std::endl;
}

const float cursorVertices[] = {
	-10, 0, 10, 0, 0, -10, 0.0, 10
};

GameClient::GameClient()
	: blockRenderer(RENDER_DIST), paused(false), firstFrame(true), FPS(0.0) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	window = glfwCreateWindow(START_WIDTH, START_HEIGHT, windowTitle, nullptr, nullptr);
	if(window == nullptr)
		throw std::runtime_error("Failed to create GLFW window");
	
	glfwMakeContextCurrent(window);
	if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
		throw std::runtime_error("Failed to initialize GLAD");
	
	glfwSetFramebufferSizeCallback(window, windowResizeCallback);
	windowResizeCallback(window, START_WIDTH, START_HEIGHT);
	glfwSwapInterval(-1);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	
	cursorProgram = loadCursorShaders();
	glGenVertexArrays(1, &cursorVAO);
	GlId cursorVBO;
	glGenBuffers(1, &cursorVBO);
	
	glBindVertexArray(cursorVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cursorVertices), cursorVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*) 0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	
	world.players.emplace_back(glm::vec3(8.0f, 40.0f, 8.0f));
	player = &world.players.back();
	
	input.init(window);
	input.capturingMouse(!paused);
	BlockRegistry::registerBlocks();
	blockRenderer.init();
	
	glfwSetTime(0.0);
}

GameClient::~GameClient() {
	glfwDestroyWindow(window);
}

constexpr float GameClient::SKY_COLOR[];

void GameClient::mainLoop() {
	glfwSetTime(0.0);
	
	int frameCounter = 0;
	
	float dt = 1 / 60.0;
	double lastFrame, lastSecond, now;
	while(!glfwWindowShouldClose(window)) {
		update(dt);
		render();
		
		now = glfwGetTime();
		dt = now - lastFrame;
		lastFrame = now;
		
		if(firstFrame) firstFrame = false;
		
		frameCounter++;
		if(now >= lastSecond + 1.0) {
			FPS = frameCounter;
			std::cout << "FPS: " << FPS << std::endl;
			frameCounter = 0;
			if(now >= lastSecond + 2.0) {
				lastSecond = now;
			} else {
				lastSecond += 1.0;
			}
		}
		
		glfwSwapBuffers(window);
	}
}

void GameClient::update(float dt) {
	glfwPollEvents();
	
	if(!paused) {
		glm::mat4 yRot = glm::mat4(1.0f);
		yRot = glm::rotate(yRot, player->orient().y, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 mvt = input.getMovement();
		player->move(dt * MVT_SPEED * glm::vec3(yRot * glm::vec4(mvt, 1.0f)));
		
		glm::vec2 mouseMvt = input.getMouseMovement();
		player->rotate(glm::vec3(-mouseMvt.y, -mouseMvt.x, 0));
	}
	
	if(input.justPressed(GLFW_KEY_ESCAPE)) {
		paused = !paused;
		input.capturingMouse(!paused);
	}
	input.clearJustPressed();
	
	bool click1 = input.justClicked(1);
	bool click2 = input.justClicked(2);
	if(click1 || click2) {
		bool hit;
		int x, y, z;
		std::tie(hit, x, y, z) = world.raycast(player->pos(), player->dirVector(), 5, !click1);
		if(hit) {
			int32_t chunkX, chunkZ;
			std::tie(chunkX, chunkZ) = world.getChunkAt(x, z);
			if(click2) {
				world.setBlock(x, y, z, Block::fromId(1));
			} else {
				world.removeBlock(x, y, z);
			}
			Chunk& chunk = world.getChunk(chunkX, chunkZ);
			blockRenderer.renderChunk(chunk, chunkX, chunkZ);
		}
	}
	input.clearJustClicked();
	
	int32_t camChunkX = floor(player->pos().x / CHUNK_SIZE);
	int32_t camChunkZ = floor(player->pos().z / CHUNK_SIZE);
	int loads = 0;
	SpiralIterator iter(camChunkX, camChunkZ);
	while(iter.withinDistance(RENDER_DIST)) {
		int x = iter.getX();
		int z = iter.getZ();
		if(!world.isChunkLoaded(x, z)) {
			Chunk& chunk = world.genChunk(x, z);
			blockRenderer.renderChunk(chunk, x, z);
			loads++;
			if(loads >= LOADS_PER_FRAME) break;
		}
		iter.next();
	}
}

void GameClient::render() {
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	float aspect = ((float) width) / height;
	glm::mat4 proj = glm::perspective(glm::radians(90.0f), aspect, 0.001f, 1000.0f);
	glm::mat4 view = globalToLocal(player->pos(), player->orient());
	
	glClearColor(SKY_COLOR[0], SKY_COLOR[1], SKY_COLOR[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	int32_t camChunkX = floor(player->pos().x / CHUNK_SIZE);
	int32_t camChunkZ = floor(player->pos().z / CHUNK_SIZE);
	blockRenderer.render(proj, view, camChunkX, camChunkZ, SKY_COLOR);
	
	glLineWidth(2.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
	glUseProgram(cursorProgram);
	glUniform2f(glGetUniformLocation(cursorProgram, "winSize"), width, height);
	glBindVertexArray(cursorVAO);
	
	glDrawArrays(GL_LINES, 0, 4);
	
	glBindVertexArray(0);
	glUseProgram(0);
	glDisable(GL_BLEND);
}
