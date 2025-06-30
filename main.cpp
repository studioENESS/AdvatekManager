#include <nlohmann/json.hpp>

#define JSON_TYPE nlohmann::json

#include "gui_elements.h"

#ifndef DEBUG
#pragma comment(linker, "/SUBSYSTEM:Windows /ENTRY:mainCRTStartup")
#endif

advatek_manager adv;

int main(int, char**)
{	
	GLFWwindow* window;

	setupWindow(window);

	// Init Advatek Manager
	applog.Clear();
	adv.refreshAdaptors();
	if (adv.networkAdaptors.size() > 0) {
		adaptor_string = adv.networkAdaptors[0];
		adv.poll();
		applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + "...\n").c_str());
		s_loopVar.lastPoll = s_loopVar.currTime;
	}

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		adv.listen();

		glfwGetWindowSize(window, &s_loopVar.window_w, &s_loopVar.window_h);

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		s_loopVar.currTime = ImGui::GetTime();
		if (s_loopVar.currTime - s_loopVar.lastPoll > s_loopVar.rePollTime) {
			// Should be a setting
			if (adv.bAutoPoll) {
				adv.softPoll();
				s_loopVar.lastPoll = s_loopVar.currTime;
				applog.AddLog(("[INFO] Auto poll adapter " + adaptor_string + "...\n").c_str());
			}
		}

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(s_loopVar.window_w, s_loopVar.window_h));
		showWindow(window);
		processUpdateRequests();
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
 
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
