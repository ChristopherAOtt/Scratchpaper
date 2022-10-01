#pragma once

#include <stdio.h>
#include <vector>
#include <memory>
#include <unistd.h> // For Sleep

#include "Window.hpp"
#include "WorldState.hpp"
#include "InputEvents.hpp"
#include "EntityStructs.hpp"
#include "Renderer.hpp"
#include "Camera.hpp"
#include "ChunkManager.hpp"
#include "MeshGenerator.hpp"
#include "SimCache.hpp"
#include "FileIO.hpp"
#include "Raytracer.hpp"
#include "Settings.hpp"

namespace VG{
	Settings updateWithSettingsFromFile(Settings settings, std::string filepath);

	class Engine{
		public:  // Public functions
			Engine();
			~Engine();

			// Settings management
			static Settings defaultSettings();
			void setDefaultSettings(Settings settings);
			void loadSettingsFromFile(std::string filepath);

			void setTargetWorld(WorldState* world_ptr);
			void initWindow();
			void initTargetWorld();
			void initResourceData(std::string filepath);
			void runMainLoop();

		private:  // Private functions
			std::vector<InputEvent> getInputEvents();
			void useProvidedSettings(Settings settings);
			void updateWidgetAssets(Renderer& world_renderer);

		private:  // Private members
			Settings m_default_settings;  // Used only to fill in values missing from a file.
			std::shared_ptr<Settings> m_settings_ptr;
			ChunkManager* m_chunk_manager;
			SimCache m_simcache;
			Renderer m_renderer;

			std::shared_ptr<Window> m_window_ptr;
			bool m_should_run;

			std::unordered_map<std::string, Fingerprint> m_widget_fingerprints;
	};
};
