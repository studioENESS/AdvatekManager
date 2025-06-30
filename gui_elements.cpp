#include "gui_elements.h"
#include "standard_json_config.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <utility>
#include <cmath>

uint32_t COL_GREY = IM_COL32(80, 80, 80, 255);
uint32_t COL_LIGHTGREY = IM_COL32(180, 180, 180, 255);
uint32_t COL_GREEN = IM_COL32(0, 180, 0, 255);
uint32_t COL_RED = IM_COL32(180, 0, 0, 255);

bool gammaSliderLock = false;
float initHue = 0.5;
int stdButtonWidth = 80;

float eness_colourcode_ouptput[16][4] = {
	{0,255,0,255},
	{255,255,0,255},
	{255,255,255,255},
	{255,0,255,255},
	{0,255,0,255},
	{255,255,0,255},
	{255,255,255,255},
	{255,0,255,255},
	{0,255,0,255},
	{255,255,0,255},
	{255,255,255,255},
	{255,0,255,255},
	{0,255,0,255},
	{255,255,0,255},
	{255,255,255,255},
	{255,0,255,255}
};

std::vector<sAdvatekDevice*> foundDevices;
std::vector<sAdvatekDevice*> flirtDevices;
std::vector<std::pair<sAdvatekDevice*, sAdvatekDevice*>> syncDevices;
sAdvatekDevice* syncDeviceVirt;

sImportOptions userImportOptions = sImportOptions();
sImportOptions virtualImportOptions = sImportOptions();

std::string adaptor_string = "None";
std::string json_device_string = "Select Device";
std::string vDeviceString = "Import";
std::string result = "";
std::string vDeviceData = "";

loopVar s_loopVar;
myTabBarFlags s_myTabBarFlags;
updateRequest s_updateRequest;
AppLog applog;

bool SliderInt8(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U8, v, &v_min, &v_max, format, flags);
}

bool SliderInt16(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U16, v, &v_min, &v_max, format, flags);
}

bool SliderInt8(const char* label, uint8_t* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U8, v, &v_min, &v_max, format, flags);
}

bool SliderInt16(const char* label, uint16_t* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
	return ImGui::SliderScalar(label, ImGuiDataType_U16, v, &v_min, &v_max, format, flags);
}

void pushStandardButtonStyleVar() {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6 * s_loopVar.scale, 6 * s_loopVar.scale));
}

void pushStandardTabStyleVar() {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3 * s_loopVar.scale, 3 * s_loopVar.scale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2 * s_loopVar.scale, 6 * s_loopVar.scale));
}

void pushControllerButtonStyleVar() {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8 * s_loopVar.scale, 4 * s_loopVar.scale));
}

void setupWindow(GLFWwindow*& window)
{
	if (!glfwInit()) exit(0);

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char* glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	GLFWmonitor * primaryMonitor = glfwGetPrimaryMonitor();
	int center_x = 50;
	int center_y = 50;

	if (primaryMonitor != NULL) {
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

		s_loopVar.xscale = mode->width / 1920.f;
		s_loopVar.yscale = mode->height / 1080.f;
		s_loopVar.window_w = s_loopVar.window_w * s_loopVar.xscale;
		s_loopVar.window_h = s_loopVar.window_h * s_loopVar.yscale;
		s_loopVar.scale = std::fmax(s_loopVar.xscale, s_loopVar.yscale);
		s_loopVar.scale = (s_loopVar.xscale + s_loopVar.yscale) / 2.f;

		center_x = (mode->width / 2) - (s_loopVar.window_w / 2);
		center_y = (mode->height / 2) - (s_loopVar.window_h / 2);
	}

	// Create window with graphics context
	window = glfwCreateWindow(s_loopVar.window_w, s_loopVar.window_h, "Advatek Manager", NULL, NULL);

	glfwSetWindowPos(window, center_x, center_y);

	if (window == NULL) exit(0);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	//TODO FIX: 
	// scaleToScreenDPI(io);

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void scaleToScreenDPI(ImGuiIO& io)
{
	ImGui::GetStyle().ScaleAllSizes(s_loopVar.scale);
	ImFontConfig fc = ImFontConfig();
	fc.OversampleH = fc.OversampleV = s_loopVar.scale;
	fc.SizePixels = 13.f * s_loopVar.scale;
	io.Fonts->AddFontDefault(&fc);
}

void pushStyleColours18(float h) {
	while (h > 1) h = h - 1;
	float offset = 0.45;
	float o = h + offset;
	while (o > 1) o = o - 1;
	//float v = 0.7;
	float v = 0;
	float grey = 0.2;
	float vOffset = 0.1;
	//float s = 0.5;
	float s = 0;

	ImGui::PushStyleColor(ImGuiCol_CheckMark,          (ImVec4)ImColor::HSV(h, s, 1));
	
	ImGui::PushStyleColor(ImGuiCol_SliderGrab,         (ImVec4)ImColor::HSV(h, s, 0.35));
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,   (ImVec4)ImColor::HSV(h, s, 0.35));

	ImGui::PushStyleColor(ImGuiCol_Button,             (ImVec4)ImColor::HSV(h, s, 0.2));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,       (ImVec4)ImColor::HSV(h, s, 0.2));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,      (ImVec4)ImColor::HSV(h, 0.5, 0.7));

	ImGui::PushStyleColor(ImGuiCol_Header,             (ImVec4)ImColor::HSV(h, s, v));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive,       (ImVec4)ImColor::HSV(h, s, v));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered,      (ImVec4)ImColor::HSV(h, 0.5, 0.7));

	ImGui::PushStyleColor(ImGuiCol_FrameBg,            (ImVec4)ImColor::HSV(h, s, grey));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive,      (ImVec4)ImColor::HSV(h, s, grey));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,     (ImVec4)ImColor::HSV(h, 0.5, 0.7));

	ImGui::PushStyleColor(ImGuiCol_Tab,                (ImVec4)ImColor::HSV(h, s, v));
	ImGui::PushStyleColor(ImGuiCol_TabActive,          (ImVec4)ImColor::HSV(h, 0.5, 0.7));
	ImGui::PushStyleColor(ImGuiCol_TabHovered,         (ImVec4)ImColor::HSV(h, s, grey));
	ImGui::PushStyleColor(ImGuiCol_TabUnfocused,       (ImVec4)ImColor::HSV(h, s, v));
	ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, (ImVec4)ImColor::HSV(h, s, v));
	ImGui::PushStyleColor(ImGuiCol_Separator,          (ImVec4)ImColor::HSV(h, 0.5, 0.7));
}

void showResult(std::string& result) {
	if (result.empty()) return;
	
	
	ImVec2 center = { (195.f * s_loopVar.xscale) + s_loopVar.window_w / 2, (50.f * s_loopVar.yscale) + s_loopVar.window_h / 2 };

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f * s_loopVar.scale, 0.5f * s_loopVar.scale));
	ImGui::SetNextWindowSize(ImVec2(400 * s_loopVar.xscale, NULL));

	ImGui::OpenPopup("Result");

	if (ImGui::BeginPopupModal("Result", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// First line is header
		std::vector<std::string> lines = splitter("\n", result);

		ImGui::Text(lines[0].c_str());
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		for (std::vector<std::string>::size_type i = 1; i != lines.size(); i++) {
			ImGui::TextWrapped(lines[i].c_str());
		}

		ImGui::PopStyleVar();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
		ImGui::TextWrapped("Please click the Update Network or Update Settings to save these changes to the controller.");
		ImGui::PopStyleColor();

		ImGui::Spacing();
		if (ImGui::Button("OK", ImVec2(stdButtonWidth * s_loopVar.scale, 0))) {
			result = std::string("");
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

}

void button_update_controller_settings(sAdvatekDevice* device) {
	if (ImGui::Button("Update Settings"))
	{
		adv.updateConnectedDevice(device);
	}
}

void button_open_close_all() {
	if (ImGui::Button("Open All", ImVec2(stdButtonWidth * s_loopVar.scale, 0)))
		s_updateRequest.openTabs = 1;
	ImGui::SameLine();
	if (ImGui::Button("Close All", ImVec2(stdButtonWidth * s_loopVar.scale, 0)))
		s_updateRequest.openTabs = 0;
}

void colouredText(const char* txt, uint32_t color) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextWrapped(txt);
	ImGui::PopStyleColor();
}

void importUI(sAdvatekDevice* device, sImportOptions& importOptions) {

	ImVec2 center = { (145.f *s_loopVar.xscale) + s_loopVar.window_w/2, (50.f * s_loopVar.yscale) + s_loopVar.window_h/2 };

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f * s_loopVar.scale, 0.5f * s_loopVar.scale));
	ImGui::SetNextWindowSize(ImVec2(300 * s_loopVar.xscale, NULL));

	if (ImGui::BeginPopupModal("What needs importing?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		// Read in devices
		JSON_TYPE pt_json_devices;
		std::vector<JSON_TYPE> loadedJsonDevices;
		std::vector<std::string> jsonDeviceNames;

		//std::stringstream ss_json_devices;
		//ss_json_devices << importOptions.json;
		//pt_json_devices << ss_json_devices;

		pt_json_devices = JSON_TYPE::parse(importOptions.json);
		
		if (pt_json_devices.count("advatek_devices") > 0) {
			for (auto& json_device : pt_json_devices["advatek_devices"]) {
				std::string sModelName;
				json_device["Model"].get_to(sModelName);
				std::string sNickName;
				json_device["Nickname"].get_to(sNickName);
				jsonDeviceNames.emplace_back(std::string(sModelName + " " + sNickName));
			}
		}
		else
		{
			std::string sModelName;
			pt_json_devices["Model"].get_to(sModelName);
			std::string sNickName;
			pt_json_devices["Nickname"].get_to(sNickName);
			jsonDeviceNames.emplace_back(std::string(sModelName + " " + sNickName));
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		if (pt_json_devices.is_array())
		{
			if (pt_json_devices.size() > 1) {
				if (ImGui::BeginCombo("###jsondevices", json_device_string.c_str(), 0))
				{
					for (int n = 0; n < jsonDeviceNames.size(); n++)
					{
						const bool is_selected = (s_loopVar.selectedNewImportIndex == n);
						if (ImGui::Selectable(jsonDeviceNames[n].c_str(), is_selected))
						{
							json_device_string = jsonDeviceNames[n];
							s_loopVar.pt_json_device = pt_json_devices[n];
						}
					}
					ImGui::EndCombo();
				}
				ImGui::Spacing();
			}
			else {
				s_loopVar.pt_json_device = pt_json_devices[0];
			}
		}
		else
		{
			s_loopVar.pt_json_device = pt_json_devices;
		}

		ImGui::Checkbox("Network", &importOptions.network);
		ImGui::Checkbox("Ethernet Control (Mapping)", &importOptions.ethernet_control);
		ImGui::Checkbox("DMX512 Outputs", &importOptions.dmx_outputs);
		ImGui::Checkbox("LED Settings", &importOptions.led_settings);
		ImGui::Checkbox("Nickname", &importOptions.nickname);
		ImGui::Checkbox("Fan On Temp", &importOptions.fan_on_temp);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PopStyleVar();

		if (ImGui::Button("Import", ImVec2(stdButtonWidth * s_loopVar.scale, 0))) {
			std::stringstream jsonStringStream;
			//write_json(jsonStringStream, s_loopVar.pt_json_device);
			jsonStringStream << s_loopVar.pt_json_device;
			importOptions.json = jsonStringStream.str();
			importOptions.userSet = true;
			ImGui::CloseCurrentPopup();
			s_loopVar.selectedNewImportIndex = -1;
			s_loopVar.pt_json_device.clear();
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(stdButtonWidth * s_loopVar.scale, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
}

void button_import_export_JSON(sAdvatekDevice* device) {

	if (ImGui::Button("Copy"))
	{
		adv.copyToMemoryDevice(device);
		applog.AddLog("[INFO] Copied data from %s (%s)\n", device->Nickname, ipStr(device->CurrentIP).c_str());
	}

	ImGui::SameLine();

	if (adv.memoryDevices.size() == 1) {
		if (ImGui::Button("Paste"))
		{
			adv.getJSON(adv.memoryDevices[0], userImportOptions);
			ImGui::OpenPopup("What needs importing?");
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Import JSON")) {
		auto path = pfd::open_file("Select a file", ".", { "JSON Files", "*.json *.JSON" }).result();
		if (!path.empty()) {
			applog.AddLog("[INFO] Loading JSON file from:\n%s\n", path.at(0).c_str());
			JSON_TYPE json_devices;
			std::ifstream file;
			file.open(path.at(0));
			std::stringstream buffer;
			buffer << file.rdbuf();
			json_devices= JSON_TYPE::parse(buffer);

			std::string result = adv.validateJSON(json_devices);
			if (!result.empty()) {
				applog.AddLog("[ERROR] Not a valid JSON file.\n");
			}
			else {
				std::stringstream jsonStringStream;
				//write_json(jsonStringStream, json_devices);
				jsonStringStream << json_devices;

				userImportOptions.json = jsonStringStream.str();

				ImGui::OpenPopup("What needs importing?");
			}
		}
	}

	// Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f * s_loopVar.scale, 0.5f * s_loopVar.scale));

	importUI(device, userImportOptions);

	if (userImportOptions.userSet) {
		result = adv.importJSON(device, userImportOptions);
		userImportOptions = sImportOptions();
	}

	ImGui::SameLine();

	if (ImGui::Button("Export JSON"))
	{
		std::stringstream ss;
		ss << device->Nickname << ".json";
		std::string filename(ss.str());
		std::replace(filename.begin(), filename.end(), ' ', '_');

		auto path = pfd::save_file("Select a file", filename).result();
		if (!path.empty()) {
			adv.exportJSON(device, path);
		}
	}
}

std::stringstream Title;
bool b_setTest, testModeEnessColourOuputs = NULL;

void showDevices(std::vector<sAdvatekDevice*>& devices, bool isConnected) {

	ImGui::Spacing();
	
	for (int i = 0; i < devices.size(); i++) {
		pushStyleColours18(devices[i]->idCol[0]);
		ImGui::Separator();
		bool deviceInRange = adv.ipInRange(adaptor_string, devices[i]);
		std::string modelName((char*)devices[i]->Model);
		modelName.append("          ").resize(19);
		modelName.append(" ");

		Title.str(std::string());
		Title.clear();
		Title << " " << modelName.c_str() << devices[i]->Firmware << "	";
		if (isConnected) {
			Title << ipStr(devices[i]->CurrentIP);
		}
		else {
			Title << ipStr(devices[i]->StaticIP);
		}
		Title.setf(std::ios::fixed, std::ios::floatfield);
		Title.precision(2);
		if (isConnected) {
			float v = (float)devices[i]->VoltageBanks[0] * 0.1;
			std::string voltage = "";
			if (v < 10) {
				voltage.append("0");
			}
			voltage.append(std::to_string(v)).resize(5);
			Title << "	" << (float)devices[i]->Temperature * 0.1 << " °C" << "	" << voltage << " V   " << devices[i]->Nickname;
		}
		else {
			Title << "	" << (float)devices[i]->Temperature * 0.1 << " °C" << "	" << "00.00 V   " << devices[i]->Nickname;
		}
		Title << "###" << devices[i]->uid;

		if (s_loopVar.open_action != -1)
			ImGui::SetNextItemOpen(s_loopVar.open_action != 0);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f * s_loopVar.scale, 8.f * s_loopVar.scale));
		bool node_open = ImGui::TreeNodeEx(Title.str().c_str(), ImGuiSelectableFlags_SpanAllColumns);
		ImGui::PopStyleVar();

		if (node_open)
		{
			pushControllerButtonStyleVar();
			//ImGui::Spacing();

			if (isConnected) {
				if (devices[i]->ProtVer >= 8) {
					if (ImGui::Button("ID"))
					{
						adv.identifyDevice(i, 20);
					}
					ImGui::SameLine();
				}
			}
			else {
				if (ImGui::Button("Delete"))
				{
					s_updateRequest.clearVirtualDeviceIndex = i;
				}
				ImGui::SameLine();
			}

			button_import_export_JSON(devices[i]);

			if (isConnected) {
				ImGui::SameLine();
				if (ImGui::Button("New Virtual Device"))
				{
					adv.copyToNewVirtualDevice(adv.connectedDevices[i]);
					applog.AddLog("[INFO] Copied controller %s %s to new virtual device.\n", adv.connectedDevices[i]->Nickname, ipStr(adv.connectedDevices[i]->CurrentIP).c_str());
				}
			}

			/* Just use the copy paste function
			else {
				
				if (adv.connectedDevices.size() > 1) {
					ImGui::PushItemWidth(240 * s_loopVar.scale);
					if (ImGui::BeginCombo("###senddevice", "Copy to Connected Device", 0))
					{
						for (int n = 0; n < adv.connectedDevices.size(); n++)
						{
							if (ImGui::Selectable(ipStr(adv.connectedDevices[n]->StaticIP).append(" ").append(adv.connectedDevices[n]->Nickname).c_str()))
							{
								adv.copyDevice(adv.virtualDevices[i], adv.connectedDevices[n], false);
								applog.AddLog("[INFO] Copied virtual controller to new device  %s %s.\n", adv.connectedDevices[i]->Nickname, ipStr(adv.connectedDevices[i]->CurrentIP).c_str());
							}
						}
						ImGui::EndCombo();
					}
					ImGui::PopItemWidth();
				}
			}*/

			ImGui::PopStyleVar(); // ControllerButtons Top
			//ImGui::Spacing();
			//ImGui::Separator();
			ImGui::Spacing();

			ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None);

			if (ImGui::BeginTabItem("Network", 0, s_myTabBarFlags.network))
			{
				pushStandardTabStyleVar();
				devices[i]->openTab = 1;
				ImGui::Spacing();

				if (isConnected && !deviceInRange) {
					ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
					ImGui::Text("The IP address settings are not compatible with your adaptor settings.\nChange the IP settings on the device to access all settings.");
					ImGui::PopStyleColor();
					ImGui::Spacing();
				}

				ImGui::AlignTextToFramePadding();
				ImGui::Text("Static IP Address:");

				ImGui::PushItemWidth(30 * s_loopVar.scale);

				ImGui::InputScalar(".##CurrentIP0", ImGuiDataType_U8, &devices[i]->StaticIP[0], 0, 0, 0);

				ImGui::SameLine();
				ImGui::InputScalar(".##CurrentIP1", ImGuiDataType_U8, &devices[i]->StaticIP[1], 0, 0, 0);

				ImGui::SameLine();
				ImGui::InputScalar(".##CurrentIP2", ImGuiDataType_U8, &devices[i]->StaticIP[2], 0, 0, 0);

				ImGui::SameLine();
				ImGui::InputScalar("##CurrentIP3", ImGuiDataType_U8, &devices[i]->StaticIP[3], 0, 0, 0);


				ImGui::Text("Static Subnet Mask:");

				ImGui::InputScalar(".##CurrentSM0", ImGuiDataType_U8, &devices[i]->StaticSM[0], 0, 0, 0);

				ImGui::SameLine();
				ImGui::InputScalar(".##CurrentSM1", ImGuiDataType_U8, &devices[i]->StaticSM[1], 0, 0, 0);

				ImGui::SameLine();
				ImGui::InputScalar(".##CurrentSM2", ImGuiDataType_U8, &devices[i]->StaticSM[2], 0, 0, 0);

				ImGui::SameLine();
				ImGui::InputScalar("##CurrentSM3", ImGuiDataType_U8, &devices[i]->StaticSM[3], 0, 0, 0);

				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("IP Type: "); ImGui::SameLine();
				int tempDHCP = (int)devices[i]->DHCP;
				if (ImGui::RadioButton("DHCP ", &tempDHCP, 1)) {
					devices[i]->DHCP = 1;
				} ImGui::SameLine();
				if (ImGui::RadioButton("Static ", &tempDHCP, 0)) {
					devices[i]->DHCP = 0;
				}
				
				ImGui::PopStyleVar(2); // Tab
				ImGui::EndTabItem();
			}

			if (!isConnected || isConnected && deviceInRange) {
				if (ImGui::BeginTabItem("Ethernet Control", 0, s_myTabBarFlags.ethernet))
				{
					pushStandardTabStyleVar();
					devices[i]->openTab = 2;
					ImGui::Spacing();

					int tempProtocol = (int)devices[i]->Protocol;
					if (ImGui::RadioButton("ArtNet ", &tempProtocol, 1)) {
						devices[i]->Protocol = 1;
					} ImGui::SameLine();
					if (ImGui::RadioButton("sACN (E1.31) ", &tempProtocol, 0)) {
						devices[i]->Protocol = 0;
					}
					ImGui::SameLine();
					bool tempHoldLastFrame = (bool)devices[i]->HoldLastFrame;
					if (ImGui::Checkbox("Hold LastFrame ", &tempHoldLastFrame)) {
						devices[i]->HoldLastFrame = tempHoldLastFrame;
					}

					bool tempSimpleConfig = (bool)devices[i]->SimpleConfig;
					if (ImGui::Checkbox("Simple Config ", &tempSimpleConfig)) {
						devices[i]->SimpleConfig = tempSimpleConfig;
					}
					
					ImGui::PushItemWidth(50 * s_loopVar.scale);
					if ((bool)devices[i]->SimpleConfig) {
						ImGui::Spacing();
						if (ImGui::InputScalar("Start Universe", ImGuiDataType_U16, &devices[i]->OutputUniv[0], 0, 0, 0)) {
							adv.process_simple_config(devices[i]);
						}
						if (ImGui::InputScalar("Start Channel ", ImGuiDataType_U16, &devices[i]->OutputChan[0], 0, 0, 0)) {
							adv.process_simple_config(devices[i]);
						}
						if (ImGui::InputScalar("Pixels Per Output", ImGuiDataType_U16, &devices[i]->OutputPixels[0], 0, 0, 0)) {
							adv.process_simple_config(devices[i]);
						}
					}
					else {
						ImGui::SameLine();
						ImGui::Checkbox("Automatic Sequence Channels ", &devices[i]->autoChannels);

						if (devices[i]->autoChannels) {
							adv.auto_sequence_channels(devices[i]);
						}

						bool* tempReversed = new bool[devices[i]->NumOutputs];
						uint16_t* tempEndUniverse = new uint16_t[devices[i]->NumOutputs];
						uint16_t* tempEndChannel = new uint16_t[devices[i]->NumOutputs];

						if (ImGui::BeginTable("advancedTable", 11))
						{
							ImGui::TableSetupColumn(" ");
							ImGui::TableSetupColumn("Start\nUniverse");
							ImGui::TableSetupColumn("Start\nChannel");
							ImGui::TableSetupColumn("End\nUniverse");
							ImGui::TableSetupColumn("End\nChannel");
							ImGui::TableSetupColumn("Num\nPixels");
							ImGui::TableSetupColumn("Null\nPixels");
							ImGui::TableSetupColumn("Zig\nZag");
							ImGui::TableSetupColumn("Group");
							ImGui::TableSetupColumn("Intensity\nLimit");
							ImGui::TableSetupColumn("Reversed");
							ImGui::TableHeadersRow();

							for (int output = 0; output < devices[i]->NumOutputs * 0.5; output++)
							{
								ImGui::TableNextRow();

								adv.setEndUniverseChannel(devices[i]->OutputUniv[output], devices[i]->OutputChan[output], devices[i]->OutputPixels[output], devices[i]->OutputGrouping[output], tempEndUniverse[output], tempEndChannel[output], devices[i]->CurrentDriverType);
								ImGui::PushID(output);

								ImGui::TableNextColumn();
								ImGui::AlignTextToFramePadding();
								ImGui::Text("Output %i", output + 1); ImGui::TableNextColumn();
								if (ImGui::InputScalar("##StartUniv", ImGuiDataType_U16, &devices[i]->OutputUniv[output], 0, 0, 0)) {
									if (devices[i]->OutputUniv[output] == 0) devices[i]->OutputUniv[output] = 1;
								}; ImGui::TableNextColumn();
								if (ImGui::InputScalar("##StartChan", ImGuiDataType_U16, &devices[i]->OutputChan[output], 0, 0, 0)) {
									if (devices[i]->OutputChan[output] == 0) devices[i]->OutputChan[output] = 1;
								}; ImGui::TableNextColumn();
								ImGui::InputScalar("##EndUniv", ImGuiDataType_U16, &tempEndUniverse[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##EndChan", ImGuiDataType_U16, &tempEndChannel[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##NumPix", ImGuiDataType_U16, &devices[i]->OutputPixels[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##NullPix", ImGuiDataType_U8, &devices[i]->OutputNull[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##ZigZag", ImGuiDataType_U16, &devices[i]->OutputZig[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("##Group", ImGuiDataType_U16, &devices[i]->OutputGrouping[output], 0, 0, 0); ImGui::TableNextColumn();
								ImGui::InputScalar("%##BrightLim", ImGuiDataType_U8, &devices[i]->OutputBrightness[output], 0, 0, 0); ImGui::TableNextColumn();
								tempReversed[output] = (bool)devices[i]->OutputReverse[output];
								if (ImGui::Checkbox("##Reversed", &tempReversed[output])) {
									devices[i]->OutputReverse[output] = (uint8_t)tempReversed[output];
								}
								ImGui::PopID();

							}
							ImGui::EndTable();
						}

						delete[] tempReversed;
						delete[] tempEndUniverse;
						delete[] tempEndChannel;

					} // End Else/If Simple Config

					ImGui::PopItemWidth();

					ImGui::Spacing();
					ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
					ImGui::Text("Note: Specified Maximum Pixels Per Output is %i", devices[i]->MaxPixPerOutput);
					ImGui::PopStyleColor();
					ImGui::PopStyleVar(2); // TabStyle
					ImGui::EndTabItem();
				}
				if (devices[i]->NumDMXOutputs > 0) {
					if (ImGui::BeginTabItem("DMX512 Outputs", 0, s_myTabBarFlags.dmx512))
					{
						pushStandardTabStyleVar();
						devices[i]->openTab = 3;
						ImGui::Spacing();

						ImGui::PushItemWidth(50 * s_loopVar.scale);

						for (int DMXoutput = 0; DMXoutput < devices[i]->NumDMXOutputs; DMXoutput++) {
							ImGui::PushID(DMXoutput);
							devices[i]->TempDmxOutOn[DMXoutput] = (bool)devices[i]->DmxOutOn[DMXoutput];

							ImGui::AlignTextToFramePadding();
							ImGui::Text("Output %i ", DMXoutput + 1);
							ImGui::SameLine();

							if (ImGui::Checkbox("Enabled ", &devices[i]->TempDmxOutOn[DMXoutput])) {
								devices[i]->DmxOutOn[DMXoutput] = (uint8_t)devices[i]->TempDmxOutOn[DMXoutput];
							}

							ImGui::SameLine();
							ImGui::InputScalar("Universe ", ImGuiDataType_U16, &devices[i]->DmxOutUniv[DMXoutput], 0, 0, 0);
							ImGui::PopID();
						}

						ImGui::PopItemWidth();
						ImGui::PopStyleVar(2); // TabStyle
						ImGui::EndTabItem();
					}
				}

				if (ImGui::BeginTabItem("LEDs", 0, s_myTabBarFlags.leds))
				{
					pushStandardTabStyleVar();
					devices[i]->openTab = 4;
					ImGui::Spacing();

					ImGui::PushItemWidth(120 * s_loopVar.scale);
					
					std::string driver_string = devices[i]->DriverNames[devices[i]->CurrentDriver];
					int currentDriver = adv.getDriverSortedIndex(devices[i]);
					if (ImGui::BeginCombo("Pixel IC", driver_string.c_str(), 0))
					{
						for (int n = 0; n < devices[i]->NumDrivers; n++)
						{
							const bool driver_selected = (currentDriver == n);
							if (ImGui::Selectable(devices[i]->DriversSorted[n].second, driver_selected))
							{
								driver_string = devices[i]->DriversSorted[n].second;
								devices[i]->CurrentDriver = devices[i]->DriversSorted[n].first;
							}
						}
						ImGui::EndCombo();
					}

					ImGui::Combo("Clock Speed", &devices[i]->CurrentDriverSpeed, DriverSpeedsMhz, 12);

					int tempAllColOrder = devices[i]->OutputColOrder[0];
					if (ImGui::Combo("RGB Order ##all", &tempAllColOrder, RGBW_Order, 24)) {
						for (int output = 0; output < devices[i]->NumOutputs * 0.5; output++) {
							devices[i]->OutputColOrder[output] = (uint8_t)tempAllColOrder;
						}
					}

					bool tempExpanded = (bool)devices[i]->CurrentDriverExpanded;
					if (ImGui::Checkbox("Expanded Mode", &tempExpanded)) {
						devices[i]->CurrentDriverExpanded = (uint8_t)tempExpanded;
					}

					ImGui::Spacing();
					ImGui::Separator(); // Move to new column?
					ImGui::Spacing();

					ImGui::Text("Gamma Correction");
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
					ImGui::Text("(Only applied to chips that are higher then 8bit...)");
					ImGui::PopStyleColor();
					ImGui::Spacing();

					ImGui::PushItemWidth(60 * s_loopVar.scale);
					if (ImGui::SliderFloat("R ", &devices[i]->Gammaf[0], 1.0, 3.0, "%.01f")) {
						devices[i]->Gamma[0] = (int)(devices[i]->Gammaf[0] * 10);
						if (gammaSliderLock) {
							devices[i]->Gamma[1] = devices[i]->Gamma[0];
							devices[i]->Gamma[2] = devices[i]->Gamma[0];
							devices[i]->Gamma[3] = devices[i]->Gamma[0];
						}
					};
					ImGui::SameLine();
					if (ImGui::SliderFloat("G ", &devices[i]->Gammaf[1], 1.0, 3.0, "%.01f")) {
						devices[i]->Gamma[1] = (int)(devices[i]->Gammaf[1] * 10);
						if (gammaSliderLock) {
							devices[i]->Gamma[0] = devices[i]->Gamma[1];
							devices[i]->Gamma[2] = devices[i]->Gamma[1];
							devices[i]->Gamma[3] = devices[i]->Gamma[1];
						}
					};
					ImGui::SameLine();
					if (ImGui::SliderFloat("B ", &devices[i]->Gammaf[2], 1.0, 3.0, "%.01f")) {
						devices[i]->Gamma[2] = (int)(devices[i]->Gammaf[2] * 10);
						if (gammaSliderLock) {
							devices[i]->Gamma[0] = devices[i]->Gamma[2];
							devices[i]->Gamma[1] = devices[i]->Gamma[2];
							devices[i]->Gamma[3] = devices[i]->Gamma[2];
						}
					};
					ImGui::SameLine();
					if (ImGui::SliderFloat("W ", &devices[i]->Gammaf[3], 1.0, 3.0, "%.01f")) {
						devices[i]->Gamma[3] = (int)(devices[i]->Gammaf[3] * 10);
						if (gammaSliderLock) {
							devices[i]->Gamma[0] = devices[i]->Gamma[3];
							devices[i]->Gamma[1] = devices[i]->Gamma[3];
							devices[i]->Gamma[2] = devices[i]->Gamma[3];
						}
					};

					devices[i]->Gammaf[0] = (float)devices[i]->Gamma[0] * 0.1;
					devices[i]->Gammaf[1] = (float)devices[i]->Gamma[1] * 0.1;
					devices[i]->Gammaf[2] = (float)devices[i]->Gamma[2] * 0.1;
					devices[i]->Gammaf[3] = (float)devices[i]->Gamma[3] * 0.1;

					ImGui::PopItemWidth();
					ImGui::SameLine();
					ImGui::Checkbox("Lock Sliders ", &gammaSliderLock);

					ImGui::PopItemWidth();
					ImGui::PopStyleVar(2); // TabStyle
					ImGui::EndTabItem();
				}

				if (isConnected) {
					if (ImGui::BeginTabItem("Test", 0, s_myTabBarFlags.test))
					{
						pushStandardTabStyleVar();
						devices[i]->openTab = 5;

						ImGui::Spacing();
						ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);
						ImGui::Text("Prior to running test mode all other setings should be saved to controller. (Press 'Update Settings')");
						ImGui::PopStyleColor();
						ImGui::Spacing();

						b_setTest = false;

						ImGui::PushItemWidth(200 * s_loopVar.scale);

						if (ImGui::Combo("Set Test ", &devices[i]->TestMode, TestModes, sizeof(TestModes) / sizeof(TestModes[0]))) {
							devices[i]->TestPixelNum = 0;
							devices[i]->TestOutputNum = 0;
							s_loopVar.b_testPixelsReady = true;
							b_setTest = true;
						}

						ImGui::SameLine();
						if (ImGui::Checkbox("Test All ", &adv.bTestAll)) {
							b_setTest = true;
						}

						if (devices[i]->TestMode == 6) {
							ImGui::SameLine();
							testModeEnessColourOuputs = devices[i]->testModeEnessColourOuputs;
							if (ImGui::Checkbox("ENESS Output Test ", &testModeEnessColourOuputs)) {
								devices[i]->testModeEnessColourOuputs = (bool)testModeEnessColourOuputs;
							}
						}

						if (devices[i]->ProtVer < 8 && devices[i]->TestMode == 8) {
							ImGui::Text("Device does not support single pixel output...");
							devices[i]->TestMode = 6;
						}

						if ((devices[i]->TestMode == 6) || (devices[i]->TestMode == 8)) {

							if (devices[i]->testModeEnessColourOuputs) {
								// Cycle colours
								if (s_loopVar.currTime - s_loopVar.lastTime > s_loopVar.testCycleSpeed) {
									devices[i]->TestOutputNum = (devices[i]->TestOutputNum) % ((int)(devices[i]->NumOutputs * 0.5)) + 1;

									devices[i]->TestCols[0] = (int)(eness_colourcode_ouptput[devices[i]->TestOutputNum-1][0] * 255);
									devices[i]->TestCols[1] = (int)(eness_colourcode_ouptput[devices[i]->TestOutputNum-1][1] * 255);
									devices[i]->TestCols[2] = (int)(eness_colourcode_ouptput[devices[i]->TestOutputNum-1][2] * 255);
									devices[i]->TestCols[3] = (int)(eness_colourcode_ouptput[devices[i]->TestOutputNum-1][3] * 255);

									s_loopVar.lastTime = s_loopVar.currTime;
									b_setTest = true;
								}
							} else {
								if (ImGui::ColorEdit4("Test Colour", devices[i]->tempTestCols)) {
									devices[i]->TestCols[0] = (int)(devices[i]->tempTestCols[0] * 255);
									devices[i]->TestCols[1] = (int)(devices[i]->tempTestCols[1] * 255);
									devices[i]->TestCols[2] = (int)(devices[i]->tempTestCols[2] * 255);
									devices[i]->TestCols[3] = (int)(devices[i]->tempTestCols[3] * 255);
									b_setTest = true;
								}
							}

							if (devices[i]->ProtVer > 7) {

								if (devices[i]->testModeCycleOuputs || devices[i]->testModeCyclePixels) {

									if (s_loopVar.currTime - s_loopVar.lastTime > s_loopVar.testCycleSpeed) {
										
										if (devices[i]->testModeCycleOuputs && s_loopVar.b_testPixelsReady) {
											devices[i]->TestOutputNum = (devices[i]->TestOutputNum) % ((int)(devices[i]->NumOutputs * 0.5)) + 1;
										}

										if (devices[i]->TestMode == 8 && devices[i]->testModeCyclePixels) {
											// Set Pixel
											devices[i]->TestPixelNum = (devices[i]->TestPixelNum) % ((int)(devices[i]->OutputPixels[(int)devices[i]->TestOutputNum - 1])) + 1;

											if (devices[i]->TestPixelNum == 1) {
												s_loopVar.b_testPixelsReady = true;
											}
											else {
												s_loopVar.b_testPixelsReady = false;
											}
										}
										else {
											s_loopVar.b_testPixelsReady = true;
										}

										s_loopVar.lastTime = s_loopVar.currTime;
										b_setTest = true;
									}
								}

								ImGui::Checkbox("Cycle Outputs###CycleOut", &devices[i]->testModeCycleOuputs);
								if (devices[i]->TestMode == 8) {
									ImGui::SameLine();
									ImGui::Checkbox("Cycle Pixels###CyclePix", &devices[i]->testModeCyclePixels);
								}
								ImGui::SliderFloat("Speed", &s_loopVar.testCycleSpeed, 5.0, 0.01, "%.01f");

							}

						} // End ProtVer 8+

						if (devices[i]->ProtVer > 7) {

							if (SliderInt8("Output (All 0)", &devices[i]->TestOutputNum, 0, (devices[i]->NumOutputs * 0.5))) {
								b_setTest = true;
							}

							if (devices[i]->TestMode == 8) {
								if (SliderInt16("Pixels (All 0)", &devices[i]->TestPixelNum, 0, (devices[i]->OutputPixels[(int)devices[i]->TestOutputNum - 1]))) {
									b_setTest = true;
								}
							}
						}

						if (b_setTest) adv.setTest(devices[i]);

						ImGui::PopItemWidth();
						ImGui::PopStyleVar(2); // TabStyle
						ImGui::EndTabItem();

					} // End Test Tab
				}

				if (ImGui::BeginTabItem("Misc", 0, s_myTabBarFlags.misc))
				{
					pushStandardTabStyleVar();
					devices[i]->openTab = 6;
					ImGui::Spacing();

					if (isConnected) {
						ImGui::Text("MAC: %s", macStr(devices[i]->Mac).c_str());
					}
					else {
						ImGui::AlignTextToFramePadding();
						ImGui::Text("MAC: ");
						ImGui::PushItemWidth(25 * s_loopVar.scale);
						ImGui::SameLine(); ImGui::InputScalar("###Mac01", ImGuiDataType_U8, &devices[i]->Mac[0], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac02", ImGuiDataType_U8, &devices[i]->Mac[1], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac03", ImGuiDataType_U8, &devices[i]->Mac[2], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac04", ImGuiDataType_U8, &devices[i]->Mac[3], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac05", ImGuiDataType_U8, &devices[i]->Mac[4], 0, 0, "%02X");
						ImGui::SameLine(); ImGui::InputScalar("###Mac06", ImGuiDataType_U8, &devices[i]->Mac[5], 0, 0, "%02X");
						ImGui::PopItemWidth();
					}

					ImGui::Text("Protocol Version: %s", std::to_string(devices[i]->ProtVer).c_str());

					ImGui::PushItemWidth(200 * s_loopVar.scale);
					char sName[40];
					memcpy(sName, devices[i]->Nickname, 40);
					if (ImGui::InputText("Nickname", sName, 40)) {
						memset(devices[i]->Nickname, 0, 40);
						strcpy(devices[i]->Nickname, sName);
					}
					ImGui::PopItemWidth();

					if (devices[i]->MaxTargetTemp != 255) {
						// Device is equiped with fan controller
						ImGui::PushItemWidth(30 * s_loopVar.scale);
						uint8_t tempMaxTargetTemp = devices[i]->MaxTargetTemp;
						if (ImGui::InputScalar("Fan Control On Temp", ImGuiDataType_U8, &tempMaxTargetTemp, 0, 0, 0)) {
							if (tempMaxTargetTemp == 255) tempMaxTargetTemp = 0;
							devices[i]->MaxTargetTemp = tempMaxTargetTemp;
						}
						ImGui::PopItemWidth();
					}

					if (isConnected) {
						int* tempVoltage = new int[devices[i]->NumBanks];

						for (int bank = 0; bank < devices[i]->NumBanks; bank++) {
							ImGui::PushID(bank);
							ImGui::Text("Bank %i: %.2f V", bank + 1, ((float)devices[i]->VoltageBanks[bank] / 10.f));
							ImGui::PopID();
						}
					}
					ImGui::PopStyleVar(2); // TabStyle
					ImGui::EndTabItem();

				}
			}

			ImGui::EndTabBar();

			ImGui::Spacing();
			ImGui::Spacing();

			pushControllerButtonStyleVar();
			if (isConnected && !deviceInRange) {
				if (ImGui::Button("Update Network"))
				{
					adv.bc_networkConfig(devices[i]);
					s_updateRequest.poll = true;
				}
				ImGui::SameLine();
			}
			else if (isConnected) {
				button_update_controller_settings(devices[i]);
				ImGui::SameLine();
			}

			if (isConnected) {
				if (ImGui::Button("Cancel###ClearDevice", ImVec2(stdButtonWidth * s_loopVar.scale, 0))) {
					s_updateRequest.clearConnectedDeviceIndex = i;
				}
				ImGui::SameLine();
			}

			if (devices.size() > 1) {
				if (ImGui::Button("Open All###Tabs", ImVec2(stdButtonWidth * s_loopVar.scale, 0)))
				{
					s_updateRequest.openTabs = 1;
					switch (devices[i]->openTab) {
					case 1:
						s_myTabBarFlags.select_network();
						break;
					case 2:
						s_myTabBarFlags.select_ethernet();
						break;
					case 3:
						s_myTabBarFlags.select_dmx512();
						break;
					case 4:
						s_myTabBarFlags.select_leds();
						break;
					case 5:
						s_myTabBarFlags.select_test();
						break;
					case 6:
						s_myTabBarFlags.select_misc();
						break;
					default:
						break;
					}
				}
			}

			ImGui::PopStyleVar(); // ControllerButtons Bottom
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::TreePop();
		}
		ImGui::PopStyleColor(18);
	}
}

void showSyncDevice(sAdvatekDevice* vdevice, bool& canSyncAll, bool& inSyncAll)
{
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	colouredText("(Virtual Device)", COL_GREY);  ImGui::SameLine();
	ImGui::Text(ipStr(vdevice->StaticIP).c_str()); ImGui::SameLine();
	colouredText(vdevice->Nickname, COL_LIGHTGREY);

	bool deviceInRange = adv.ipInRange(adaptor_string, vdevice);
	if (!deviceInRange) {
		colouredText("Device IP address settings not compatible with your adaptor settings.", COL_RED);
		colouredText("Change the IP settings of your adaptor.", COL_RED);
		canSyncAll = false;
		return;
	}

	foundDevices.clear();
	flirtDevices.clear();

	switch (s_loopVar.current_sync_type) {
	case 0: // Match Static IP
		flirtDevices = adv.getDevicesWithStaticIP(adv.virtualDevices, ipStr(vdevice->StaticIP));
		if (flirtDevices.size() > 1) {
			colouredText(std::string("Multiple virtual devices with static IP ").append(ipStr(vdevice->StaticIP)).c_str(), COL_RED);
			canSyncAll = false;
			return;
		}
		foundDevices = adv.getDevicesWithStaticIP(adv.connectedDevices, ipStr(vdevice->StaticIP));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				colouredText(std::string("Multiple connected devices with static IP ").append(ipStr(vdevice->StaticIP)).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with static IP ").append(ipStr(vdevice->StaticIP)).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	case 1: // Match Nickname
		flirtDevices = adv.getDevicesWithNickname(adv.virtualDevices, std::string(vdevice->Nickname));
		if (flirtDevices.size() > 1) {
			colouredText(std::string("Multiple virtual devices with Nickname ").append(vdevice->Nickname).c_str(), COL_RED);
			canSyncAll = false;
			return;
		}
		foundDevices = adv.getDevicesWithNickname(adv.connectedDevices, std::string(vdevice->Nickname));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				colouredText(std::string("Multiple connected devices with Nickname ").append(vdevice->Nickname).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with Nickname ").append(vdevice->Nickname).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	case 2: // Match MAC
		flirtDevices = adv.getDevicesWithMac(adv.virtualDevices, macStr(vdevice->Mac));
		if (flirtDevices.size() > 1) {
			colouredText(std::string("Multiple vuirtual devices with MAC address ").append(macStr(vdevice->Mac)).c_str(), COL_RED);
			canSyncAll = false;
			return;
		}
		foundDevices = adv.getDevicesWithMac(adv.connectedDevices, macStr(vdevice->Mac));
		if (foundDevices.size() != 1) {
			if (foundDevices.size()) {
				// This should never happen :)
				colouredText(std::string("Whoah!! Multiple connected devices with MAC address ").append(macStr(vdevice->Mac)).c_str(), COL_RED);
			}
			else {
				colouredText(std::string("No connected device with MAC address ").append(macStr(vdevice->Mac)).c_str(), COL_RED);
			}
			canSyncAll = false;
			return;
		}
		break;
	default:
		return;
		break;
	}

	if (adv.deviceCompatible(vdevice, foundDevices[0])) {
		if (adv.devicesInSync(vdevice, foundDevices[0])) {
			colouredText("Device in sync ...", COL_GREEN);
		}
		else {
			inSyncAll = false;
			syncDevices.emplace_back(vdevice, foundDevices[0]);
			if (ImGui::Button("Update Connected Device"))
			{
				adv.updateConnectedDevice(vdevice, foundDevices[0]);
			}
		}
		return;
	}
	else {
		colouredText("Device not compatable ...", COL_RED);
		canSyncAll = false;
		return;
	}
}

std::stringstream tabTitleConnected;
std::stringstream tabTitleVirtual;
bool canSyncAll, inSyncAll = NULL;

void guiLoadJSON() {
	auto path = pfd::open_file("Select a file", ".", { "JSON Files", "*.json *.JSON" }).result();
	if (!path.empty()) {
		applog.AddLog("[INFO] Loading JSON file from:\n%s\n", path.at(0).c_str());
		JSON_TYPE advatek_devices;
		std::ifstream file;
		file.open( path.at(0));
		std::stringstream buffer;
		buffer << file.rdbuf();
		advatek_devices = JSON_TYPE::parse(buffer);


		std::string result = adv.validateJSON(advatek_devices);

		if (!result.empty()) {
			applog.AddLog("[ERROR] Not a valid JSON file.\n");
		}
		else {
			std::stringstream jsonStringStream;
			jsonStringStream << advatek_devices;

			virtualImportOptions = sImportOptions();
			virtualImportOptions.json = advatek_devices.dump();
			virtualImportOptions.init = true;

			s_updateRequest.newVirtualDevice = true;
		}
	}
}

void showWindow(GLFWwindow*& window)
{
	{
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoCollapse;

		ImGui::Begin("Advatek Assistor", NULL, window_flags);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3 * s_loopVar.scale);
		pushStandardButtonStyleVar();

		pushStyleColours18(initHue);

		showResult(result);

		if (ImGui::Button("Search", ImVec2(stdButtonWidth * s_loopVar.scale, 0)))
		{
			s_updateRequest.poll = true;
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(130 * s_loopVar.scale);
		if (ImGui::BeginCombo("###Adaptor", adaptor_string.c_str(), 0))
		{
			for (int n = 0; n < adv.networkAdaptors.size(); n++)
			{
				const bool is_selected = (adv.currentAdaptor == n);
				if (ImGui::Selectable(adv.networkAdaptors[n].c_str(), is_selected))
				{
					adaptor_string = adv.networkAdaptors[n];
					adv.setCurrentAdaptor(n);
					applog.AddLog(("[INFO] Set network adaptor to " + adaptor_string + "...\n").c_str());
					s_updateRequest.poll = true;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();
		if (ImGui::Button("Refresh Adaptors"))
		{
			s_updateRequest.refreshAdaptors = true;
		}

		ImGui::Spacing();

		ImGui::PopStyleVar();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16 * s_loopVar.scale, 8 * s_loopVar.scale));
		ImGui::PopStyleColor(18);;
		pushStyleColours18(initHue);

		if (ImGui::BeginTabBar("Devices")) {
			tabTitleConnected.str(std::string());
			tabTitleVirtual.str(std::string());
			tabTitleConnected.clear();
			tabTitleVirtual.clear();
			tabTitleConnected << "Connected Devices (";
			tabTitleVirtual << "Virtual Devices (";
			tabTitleConnected << adv.connectedDevices.size();
			tabTitleVirtual << adv.virtualDevices.size();
			tabTitleConnected << ")";
			tabTitleVirtual << ")";
			tabTitleConnected << "###connectedDevices";
			tabTitleVirtual << "###virtualDevices";

			if (ImGui::BeginTabItem(tabTitleConnected.str().c_str()))
			{
				pushStandardButtonStyleVar();
				ImGui::Spacing();

				if (adv.connectedDevices.size() >= 1) {

					button_open_close_all();
					
					ImGui::SameLine();
					ImGui::PushItemWidth(150 * s_loopVar.scale);
					if (ImGui::Combo("###SortConnectedDevices", &adv.sortTypeConnected, adv.SortTypes, IM_ARRAYSIZE(adv.SortTypes))) {
						adv.sortAllDevices();
					}
					ImGui::PopItemWidth();

					std::string path;
					ImGui::SameLine();
					
					if (ImGui::BeginCombo("###ExportTypes", "Export", 0))
					{
						for (int n = 0; n < IM_ARRAYSIZE(ExportAllTypes); n++)
						{
							if (ImGui::Selectable(ExportAllTypes[n]))
							{
								switch (n) {
								case 0: //JSON
									path = pfd::save_file("Select a file", "controllerPack.json").result();
									if (!path.empty()) {
										adv.exportJSON(adv.connectedDevices, path);
									}
									break;
								case 1: // Virtual Devices Add
									s_updateRequest.connectedDevicesToVirtualDevices = true;
									break;
								case 2: // Virtual Devices Clean
									adv.clearDevices(adv.virtualDevices);
									s_updateRequest.connectedDevicesToVirtualDevices = true;
									break;
								default:
									break;
								}
							}
						}
						ImGui::EndCombo();
					}
				}

				ImGui::PopStyleVar(); // Buttons
				ImGui::Spacing();

				showDevices(adv.connectedDevices, true);

				// END Connected Devices
				ImGui::EndTabItem();
			}
			
			// -------------------------------------- START SYNC
			if (ImGui::BeginTabItem("<-"))
			{
				pushStandardTabStyleVar();
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * s_loopVar.scale, 6 * s_loopVar.scale));
				ImGui::Spacing();
				ImGui::PushItemWidth(182 * s_loopVar.scale);
				ImGui::Combo("###SyncTypes", &s_loopVar.current_sync_type, SyncTypes, IM_ARRAYSIZE(SyncTypes));
				ImGui::PopItemWidth();
				ImGui::SameLine();

				if (syncDevices.size() > 0) {
					if (ImGui::Button("Update All###top"))
					{
						for (int i = 0; i < syncDevices.size(); i++) {
							adv.updateConnectedDevice(syncDevices[i].first, syncDevices[i].second);
						}
					}
				}

				canSyncAll = true;
				inSyncAll = true;

				syncDevices.clear();

				for (int i = 0; i < adv.virtualDevices.size(); i++) {
					ImGui::PushID(i);
					showSyncDevice(adv.virtualDevices[i], canSyncAll, inSyncAll);
					ImGui::PopID();
				}

				ImGui::Spacing();

				if (/*canSyncAll &&*/ !inSyncAll) {
					ImGui::Separator();
					ImGui::Spacing();
					if (ImGui::Button("Update All###bottom"))
					{
						for (int i = 0; i < syncDevices.size(); i++) {
							adv.updateConnectedDevice(syncDevices[i].first, syncDevices[i].second);
						}
					}
					ImGui::Spacing();
				}
				ImGui::PopStyleVar(3); // TabStyle
				ImGui::EndTabItem();
			}
			// -------------------------------------- END SYNC

			if (ImGui::BeginTabItem(tabTitleVirtual.str().c_str()))
			{
				pushStandardButtonStyleVar();
				ImGui::Spacing();

				if (adv.virtualDevices.size() >= 1) {

					button_open_close_all();
					
					if (adv.virtualDevices.size() > 1) {
						ImGui::SameLine();
						if (ImGui::Combo("###SortVirtualDevices", &adv.sortTypeVirtual, adv.SortTypes, IM_ARRAYSIZE(adv.SortTypes) - 2)) {
							adv.sortAllDevices();
						}
					}

					ImGui::SameLine();
				}

				if (ImGui::BeginCombo("###NewVirtualDevice", vDeviceString.c_str(), 0))
				{
					const bool jsonAdd_selected = (vDeviceString.c_str() == "JSON (Add)");
					if (ImGui::Selectable("JSON (add)", jsonAdd_selected))
					{
						guiLoadJSON();
					}

					const bool jsonClean_selected = (vDeviceString.c_str() == "JSON (clean)");
					if (ImGui::Selectable("JSON (clean)", jsonClean_selected)) {
						s_updateRequest.clearVirtualDevices = true;
						guiLoadJSON();
					}

					for (const auto& jsonData : JSONControllers)
					{
						const bool jsonController_selected = (vDeviceString.c_str() == jsonData[0].c_str());
						if (ImGui::Selectable(jsonData[0].c_str(), jsonController_selected))
						{
							virtualImportOptions = sImportOptions();
							virtualImportOptions.json = jsonData[1];
							virtualImportOptions.init = true;
							s_updateRequest.newVirtualDevice = true;
						}
					}
					ImGui::EndCombo();
				}

				if (adv.virtualDevices.size() >= 1) {
					ImGui::SameLine();
					std::string path;
					if (ImGui::BeginCombo("###ExportTypesV", "Export", 0))
					{
						for (int n = 0; n < IM_ARRAYSIZE(ExportAllTypes) - 2; n++)
						{
							if (ImGui::Selectable(ExportAllTypes[n]))
							{
								switch (n) {
								case 0: //JSON
									path = pfd::save_file("Select a file", "controllerPack.json").result();
									if (!path.empty()) {
										adv.exportJSON(adv.virtualDevices, path);
									}
									break;
								default:
									break;
								}
							}
						}
						ImGui::EndCombo();
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear All"))
				{
					s_updateRequest.clearVirtualDevices = true;
				}

				if (adv.memoryDevices.size() == 1) {
					ImGui::SameLine();
					if (ImGui::Button("Paste", ImVec2(stdButtonWidth * s_loopVar.scale, 0)))
					{
						s_updateRequest.pasteToNewVirtualDevice = true;
					}
				}
				ImGui::PopStyleVar(); // Buttons

				ImGui::Spacing();
				showDevices(adv.virtualDevices, false);
				
				ImGui::EndTabItem();
			}
			
			ImGui::EndTabBar();
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Separator();
		
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(18);
		pushStyleColours18(initHue);
		applog.Draw("Advatek Assistor");
		ImGui::PopStyleColor(18);
		ImGui::PopStyleVar();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(window);
}

void processUpdateRequests()
{
	switch(s_updateRequest.openTabs) {
	case 1: // Open
		s_loopVar.open_action = 1;
		s_updateRequest.openTabs = -1;
		break;
	case 0: // Close
		s_loopVar.open_action = 0;
		s_updateRequest.openTabs = -1;
		break;
	default:
		s_loopVar.open_action = -1;
		s_myTabBarFlags.Clear();
		break;
	}

	if (s_updateRequest.refreshAdaptors) {
		applog.AddLog("[INFO] Refreshing network adaptors...\n");
		s_updateRequest.refreshAdaptors = false;
		adv.refreshAdaptors();
		if (adv.networkAdaptors.size() > 0) {
			adaptor_string = adv.networkAdaptors[0];
			adv.poll();
			applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + "...\n").c_str());
			s_loopVar.lastPoll = s_loopVar.currTime;
		}
		else {
			adaptor_string = "None";
		}
	}

	if (s_updateRequest.poll) {
		adv.poll();
		applog.AddLog(("[INFO] Polling using network adaptor " + adaptor_string + "...\n").c_str());
		s_updateRequest.poll = false;
	}

	if (s_updateRequest.clearVirtualDevices) {
		adv.clearDevices(adv.virtualDevices);
		s_updateRequest.clearVirtualDevices = false;
		applog.AddLog("[INFO] Cleared all virtual devices.\n");
	}

	if (s_updateRequest.clearVirtualDeviceIndex >= 0) {
		applog.AddLog(("[INFO] Removing Virtual Device " + ipStr(adv.virtualDevices[s_updateRequest.clearVirtualDeviceIndex]->StaticIP).append(" ").append(adv.virtualDevices[s_updateRequest.clearVirtualDeviceIndex]->Nickname).append("\n")).c_str());
		adv.virtualDevices.erase(adv.virtualDevices.begin() + s_updateRequest.clearVirtualDeviceIndex);
		s_updateRequest.clearVirtualDeviceIndex = -1;
	}

	if (s_updateRequest.clearConnectedDeviceIndex >= 0) {
		applog.AddLog(("[INFO] Removing Connected Device " + ipStr(adv.connectedDevices[s_updateRequest.clearConnectedDeviceIndex]->StaticIP).append(" ").append(adv.connectedDevices[s_updateRequest.clearConnectedDeviceIndex]->Nickname).append("\n")).c_str());
		adv.connectedDevices.erase(adv.connectedDevices.begin() + s_updateRequest.clearConnectedDeviceIndex);
		s_updateRequest.clearConnectedDeviceIndex = -1;
	}

	if (s_updateRequest.newVirtualDevice) {
		adv.addVirtualDevice(virtualImportOptions);
		applog.AddLog("[INFO] Creating new Virtual Device(s).\n");
		s_updateRequest.newVirtualDevice = false;
	}

	if (s_updateRequest.pasteToNewVirtualDevice) {
		adv.pasteToNewVirtualDevice();
		applog.AddLog("[INFO] Pasting data into new virtual device.\n");
		s_updateRequest.pasteToNewVirtualDevice = false;
	}

	if (s_updateRequest.connectedDevicesToVirtualDevices) {
		applog.AddLog("[INFO] Copying Connected Devices to Virtual Devices...");
		// Show loading bar here?
		for (auto device : adv.connectedDevices) {
			JSON_TYPE advatek_device;
			adv.getJSON(device, advatek_device);
			sImportOptions conImportOptions = sImportOptions();
			conImportOptions.init = true;
			adv.addVirtualDevice(advatek_device, conImportOptions);
		}
		s_updateRequest.connectedDevicesToVirtualDevices = false;
	}

}

AppLog::AppLog()
{
	AutoScroll = true;
	Clear();
}

void AppLog::Clear()
{
	Buf.clear();
	LineOffsets.clear();
	LineOffsets.push_back(0);
	AddLog("[INFO] Advatek Manager v%s\n", Version);
}

void AppLog::AddLog(const char* fmt, ...)
{
	int old_size = Buf.size();
	va_list args;
	va_start(args, fmt);
	Buf.appendfv(fmt, args);
	va_end(args);
	for (int new_size = Buf.size(); old_size < new_size; old_size++)
		if (Buf[old_size] == '\n')
			LineOffsets.push_back(old_size + 1);
}

void AppLog::Draw(const char* title, bool* p_open /*= NULL*/)
{
	ImGui::PushAllowKeyboardFocus(false);

	if (!ImGui::Begin(title, p_open))
	{
		ImGui::End();
		return;
	}

	// Main window

	ImGui::Spacing();
	ImGui::Checkbox("Auto-scroll", &AutoScroll);
	ImGui::SameLine();
	bool clear = ImGui::Button("Clear", ImVec2(50 * s_loopVar.scale,0));
	ImGui::SameLine();
	bool copy = ImGui::Button("Copy", ImVec2(50 * s_loopVar.scale, 0));
	ImGui::SameLine();
	ImGui::PushItemWidth(130 * s_loopVar.scale);
	Filter.Draw("Filter");
	ImGui::PopItemWidth();

	ImGui::Spacing();
	ImGui::BeginChild("scrolling", ImVec2(0, 79 * s_loopVar.scale), false, ImGuiWindowFlags_HorizontalScrollbar);

	if (clear)
		Clear();
	if (copy)
		ImGui::LogToClipboard();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	ImGui::PushStyleColor(ImGuiCol_Text, COL_GREY);

	const char* buf = Buf.begin();
	const char* buf_end = Buf.end();
	if (Filter.IsActive())
	{
		for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
		{
			const char* line_start = buf + LineOffsets[line_no];
			const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
			if (Filter.PassFilter(line_start, line_end))
				ImGui::TextUnformatted(line_start, line_end);
		}
	}
	else
	{
		ImGuiListClipper clipper;
		clipper.Begin(LineOffsets.Size);
		while (clipper.Step())
		{
			for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				ImGui::TextUnformatted(line_start, line_end);
			}
		}
		clipper.End();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	if (AutoScroll)
		ImGui::SetScrollHereY(1.0);

	ImGui::EndChild();
	ImGui::End();

	ImGui::PopAllowKeyboardFocus();
}

void myTabBarFlags::Clear() {
	network = ImGuiTabItemFlags_None;
	ethernet = ImGuiTabItemFlags_None;
	dmx512 = ImGuiTabItemFlags_None;
	leds = ImGuiTabItemFlags_None;
	test = ImGuiTabItemFlags_None;
	misc = ImGuiTabItemFlags_None;
}

void myTabBarFlags::select_network() {
	Clear();
	network = ImGuiTabItemFlags_SetSelected;
}
void myTabBarFlags::select_ethernet() {
	Clear();
	ethernet = ImGuiTabItemFlags_SetSelected;
}
void myTabBarFlags::select_dmx512() {
	Clear();
	dmx512 = ImGuiTabItemFlags_SetSelected;
}
void myTabBarFlags::select_leds() {
	Clear();
	leds = ImGuiTabItemFlags_SetSelected;
}
void myTabBarFlags::select_test() {
	Clear();
	test = ImGuiTabItemFlags_SetSelected;
}
void myTabBarFlags::select_misc() {
	Clear();
	misc = ImGuiTabItemFlags_SetSelected;
}
