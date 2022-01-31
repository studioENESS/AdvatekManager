#include "advatek_assistor.h"

const char* DriverTypes[3] = {
		"RGB only",
		"RGBW only",
		"Either RGB or RGBW"
};	

const char* DriverSpeedsMhz[12] = {
	"0.4 MHz", // Data Only Slow
	"0.6 MHz", // Data Only Fast
	"0.8 MHz",
	"1.0 MHz",
	"1.2 MHz",
	"1.4 MHz",
	"1.6 MHz",
	"1.8 MHz",
	"2.0 MHz",
	"2.2 MHz",
	"2.5 MHz",
	"2.9 MHz"
};

const char* TestModes[9] = {
	"None(Live Control Data)",
	"RGBW Cycle",
	"Red",
	"Green",
	"Blue",
	"White",
	"Set Color",
	"Color Fade",
	"Single Pixel"
};

std::string macString(uint8_t * address) {
	std::stringstream ss;

	for (int i(0); i < 6; i++) {
		ss << std::hex << std::setw(2) << static_cast<int>(address[i]);
		if (i < 5) ss << ":";
	}

	return ss.str();
}

std::string ipString(uint8_t * address) {
	std::stringstream ss;

	for (int i(0); i < 4; i++) {
		ss << static_cast<int>(address[i]);
		if (i < 3) ss << ".";
	}

	return ss.str();
}

void insertSwapped16(std::vector<uint8_t> &dest, uint16_t* pData, int32_t size)
{
	for (int i = 0; i < size; i++)
	{
		uint16_t data = pData[i];
		dest.push_back((uint8_t)(data >> 8));
		dest.push_back((uint8_t)data);
	}
}

bool advatek_manager::deviceExist(uint8_t * Mac) {

	for (int d(0); d < advatek_manager::devices.size(); d++) {
		bool exist = true;
		for (int i(0); i < 6; i++) {
			if (advatek_manager::devices[d]->Mac[i] != Mac[i]) exist = false;
		}
		if (exist) return true;
	}

	return false;
}

/*
Windows and Linux have differing mindsets when it comes to accepting broadcast packets.
If you bind a socket to a specific protocol, port and address under Windows, you will receive
packets specifically for that protocol/port/address plus any broadcast packets on that port and subnet for that address. 

However, under Linux, if you bind a socket to a specific protocol, port and address you will only receive packets specifically 
for that protocol/port/address triplet, and will not receive broadcast packets. If you want to receive broadcast packets 
under Linux, then you have to bind a socket to the any address option.
*/
boost::asio::io_context io_context;
boost::asio::ip::udp::endpoint receiver(boost::asio::ip::address_v4::any(), AdvPort);

boost::asio::ip::udp::socket s_socket(io_context);
boost::asio::ip::udp::socket r_socket(io_context, receiver);

boost::asio::ip::tcp::resolver resolver(io_context);
boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");

void advatek_manager::send_udp_message(std::string ip_address, int port, bool b_broadcast, std::vector<uint8_t> message)
{

	boost::asio::io_service io_service;
	boost::asio::ip::udp::socket socket(io_service);
	boost::asio::ip::udp::endpoint local_endpoint;
	boost::asio::ip::udp::endpoint remote_endpoint;
	
	socket.open(boost::asio::ip::udp::v4());
	socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
	socket.set_option(boost::asio::socket_base::broadcast(b_broadcast));
	//local_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), port);
	if(b_broadcast) {
		remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::broadcast(), port);
	} else {
		remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(ip_address), port);
	}

	try {
		//socket.bind(local_endpoint);
		socket.send_to(boost::asio::buffer(message), remote_endpoint);
	} catch (boost::system::system_error e) {
		std::cout << e.what() << std::endl;
	}

/*
	//boost::asio::ip::udp::endpoint senderEndpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port);
	try {
		
		// Open the socket

		s_socket.open(boost::asio::ip::udp::v4());
		//s_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
		s_socket.set_option(boost::asio::socket_base::broadcast(b_broadcast));

		// Set which interface to use 
		//s_socket.set_option(boost::asio::ip::multicast::outbound_interface(boost::asio::ip::address::from_string(advatek_manager::networkAdaptors[currentAdaptor].c_str()).to_v4()));
		//s_socket.set_option(boost::asio::socket_base::do_not_route(true));
		//s_socket.set_option(boost::asio::socket_base::reuse_address(true));

		boost::asio::ip::udp::endpoint senderEndpoint(boost::asio::ip::address::from_string(ip_address), port);

		// And send the string... (synchronous / blocking)
		s_socket.send_to(boost::asio::buffer(message), senderEndpoint);

		printf("Message sent to %s\n", ip_address.c_str());
	
	} catch (const boost::system::system_error& ex) {
		std::cout << "Failed to send message from " << advatek_manager::networkAdaptors[currentAdaptor].c_str() << " to " <<  ip_address.c_str() << std::endl;
		std::cout << ex.what() << std::endl;
	}

	if(s_socket.is_open()){
		s_socket.close();
	}
*/
}

void advatek_manager::unicast_udp_message(std::string ip_address, std::vector<uint8_t> message)
{
	send_udp_message(ip_address, AdvPort, false, message);
}

void advatek_manager::broadcast_udp_message(std::vector<uint8_t> message)
{
	send_udp_message(AdvAdr, AdvPort, true, message);
}

void advatek_manager::updateDevice(int d) {

	auto device = advatek_manager::devices[d];
	std::vector<uint8_t> dataTape;

	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x05;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	dataTape.insert(dataTape.end(), device->Mac, device->Mac + 6);

	dataTape.push_back(device->DHCP);

	dataTape.insert(dataTape.end(), device->StaticIP, device->StaticIP + 4);
	dataTape.insert(dataTape.end(), device->StaticSM, device->StaticSM + 4);

	dataTape.push_back(device->Protocol);
	dataTape.push_back(device->HoldLastFrame);
	dataTape.push_back(device->SimpleConfig);

	insertSwapped16(dataTape, device->OutputPixels, device->NumOutputs);
	insertSwapped16(dataTape, device->OutputUniv, device->NumOutputs);
	insertSwapped16(dataTape, device->OutputChan, device->NumOutputs);

	dataTape.insert(dataTape.end(), device->OutputNull, device->OutputNull + device->NumOutputs);
	insertSwapped16(dataTape, device->OutputZig, device->NumOutputs);

	dataTape.insert(dataTape.end(), device->OutputReverse, device->OutputReverse + device->NumOutputs);
	dataTape.insert(dataTape.end(), device->OutputColOrder, device->OutputColOrder + device->NumOutputs);
	insertSwapped16(dataTape, device->OutputGrouping, device->NumOutputs);

	dataTape.insert(dataTape.end(), device->OutputBrightness, device->OutputBrightness + device->NumOutputs);
	dataTape.insert(dataTape.end(), device->DmxOutOn, device->DmxOutOn + device->NumDMXOutputs);
	insertSwapped16(dataTape, device->DmxOutUniv, device->NumDMXOutputs);

	dataTape.push_back(device->CurrentDriver);
	dataTape.push_back(device->CurrentDriverType);

	dataTape.push_back(device->CurrentDriverSpeed);
	dataTape.push_back(device->CurrentDriverExpanded);

	dataTape.insert(dataTape.end(), device->Gamma, device->Gamma + 4);
	dataTape.insert(dataTape.end(), device->Nickname, device->Nickname + 40);

	dataTape.push_back(device->MaxTargetTemp);

	unicast_udp_message(ipString(device->CurrentIP), dataTape);
}

void advatek_manager::setTest(int d) {

	auto device = advatek_manager::devices[d];

	std::vector<uint8_t> dataTape;
	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x08;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	dataTape.insert(dataTape.end(), device->Mac, device->Mac + 6);

	dataTape.push_back(device->TestMode);

	dataTape.insert(dataTape.end(), device->TestCols, device->TestCols + 4);

	dataTape.push_back(device->TestOutputNum);

	dataTape.push_back((uint8_t)(device->TestPixelNum >> 8));
	dataTape.push_back((uint8_t)device->TestPixelNum);

	//buf[23] = 0;// device->TestOutputNum;
	//buf[24] = 0;//device->TestPixelNum;
	//buf[25] = 0;//device->TestPixelNum >> 8;

	unicast_udp_message(ipString(device->CurrentIP), dataTape);
}

void advatek_manager::clearDevices() {
	for (auto device : advatek_manager::devices)
	{
		if (device)
		{
			delete device->Model;
			delete device->Firmware;
			delete[] device->OutputPixels;
			delete[] device->OutputUniv;
			delete[] device->OutputChan;
			delete[] device->OutputNull;
			delete[] device->OutputZig;
			delete[] device->OutputReverse;
			delete[] device->OutputColOrder;
			delete[] device->OutputGrouping;
			delete[] device->OutputBrightness;
			delete[] device->DmxOutOn;
			delete[] device->DmxOutUniv;
			delete[] device->DriverType;
			delete[] device->DriverSpeed;
			delete[] device->DriverExpandable;
			delete[] device->DriverNames;
			delete[] device->VoltageBanks;
			delete device;
		}
	}

	advatek_manager::devices.clear();
}

void setEndUniverseChannel(int startUniverse, int startChannel, int pixelCount, int outputGrouping, int &endUniverse, int &endChannel) {
	pixelCount *= outputGrouping;
	int pixelChannels = (3 * pixelCount); // R-G-B data
	int pixelUniverses = ((float)(startChannel + pixelChannels) / 510.f);

	endUniverse = startUniverse + pixelUniverses;
	endChannel = (startChannel + pixelChannels - 1) % 510;
}

void advatek_manager::bc_networkConfig(int d) {
	auto device = advatek_manager::devices[d];
	std::vector<uint8_t> dataTape;
	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x07;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	dataTape.insert(dataTape.end(), device->Mac, device->Mac + 6);

	dataTape.push_back(device->DHCP);

	dataTape.insert(dataTape.end(), device->StaticIP, device->StaticIP + 4);
	dataTape.insert(dataTape.end(), device->StaticSM, device->StaticSM + 4);

	broadcast_udp_message(dataTape);
}

void advatek_manager::poll() {

	advatek_manager::clearDevices();

	std::vector<uint8_t> dataTape;
	dataTape.resize(12);
	dataTape[0] = 'A';
	dataTape[1] = 'd';
	dataTape[2] = 'v';
	dataTape[3] = 'a';
	dataTape[4] = 't';
	dataTape[5] = 'e';
	dataTape[6] = 'c';
	dataTape[7] = 'h';
	dataTape[8] = 0x00;   // Null Terminator
	dataTape[9] = 0x00;   // OpCode
	dataTape[10] = 0x01;  // OpCode
	dataTape[11] = 0x08;  // ProtVer

	broadcast_udp_message(dataTape);

}

void advatek_manager::process_opPollReply(uint8_t * data) {
	sAdvatekDevice * rec_data = new sAdvatekDevice();

	memcpy(&rec_data->ProtVer, data, sizeof(uint8_t));
	data += 1;
	//std::cout << "ProtVer: " << (int)rec_data->ProtVer << std::endl;

	memcpy(&rec_data->CurrentProtVer, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->Mac, data, 6);
	data += 6;

	memcpy(&rec_data->ModelLength, data, sizeof(uint8_t));
	data += 1;

	rec_data->Model = new uint8_t[rec_data->ModelLength];
	memcpy(rec_data->Model, data, sizeof(uint8_t)*rec_data->ModelLength);
	data += rec_data->ModelLength;

	memcpy(&rec_data->HwRev, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->MinAssistantVer, data, sizeof(uint8_t) * 3);
	data += 3;

	memcpy(&rec_data->LenFirmware, data, sizeof(uint8_t));
	data += 1;

	rec_data->Firmware = new uint8_t[rec_data->LenFirmware];
	memcpy(rec_data->Firmware, data, sizeof(uint8_t)*rec_data->LenFirmware);
	data += rec_data->LenFirmware;

	memcpy(&rec_data->Brand, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->CurrentIP, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(rec_data->CurrentSM, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(&rec_data->DHCP, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->StaticIP, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(rec_data->StaticSM, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(&rec_data->Protocol, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->HoldLastFrame, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->SimpleConfig, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->MaxPixPerOutput, data, sizeof(uint16_t));
	data += 2;
	bswap_16(rec_data->MaxPixPerOutput);

	memcpy(&rec_data->NumOutputs, data, sizeof(uint8_t));
	data += 1;
	rec_data->OutputPixels = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputPixels, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputPixels[i]);
	}

	rec_data->OutputUniv = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputUniv, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputUniv[i]);
	}

	rec_data->OutputChan = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputChan, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputChan[i]);
	}

	rec_data->OutputNull = new uint8_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputNull, data, sizeof(uint8_t)*rec_data->NumOutputs);
	data += rec_data->NumOutputs;

	rec_data->OutputZig = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputZig, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputZig[i]);
	}

	rec_data->OutputReverse = new uint8_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputReverse, data, sizeof(uint8_t)*rec_data->NumOutputs);
	data += rec_data->NumOutputs;

	rec_data->OutputColOrder = new uint8_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputColOrder, data, sizeof(uint8_t)*rec_data->NumOutputs);
	data += rec_data->NumOutputs;

	rec_data->OutputGrouping = new uint16_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputGrouping, data, sizeof(uint16_t)*rec_data->NumOutputs);
	data += (rec_data->NumOutputs * 2);

	for (int i(0); i < (int)rec_data->NumOutputs; i++) {
		bswap_16(rec_data->OutputGrouping[i]);
	}

	rec_data->OutputBrightness = new uint8_t[rec_data->NumOutputs];
	memcpy(rec_data->OutputBrightness, data, sizeof(uint8_t)*rec_data->NumOutputs);
	data += rec_data->NumOutputs;

	memcpy(&rec_data->NumDMXOutputs, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->ProtocolsOnDmxOut, data, sizeof(uint8_t));
	data += 1;

	rec_data->DmxOutOn = new uint8_t[rec_data->NumDMXOutputs];
	memcpy(rec_data->DmxOutOn, data, sizeof(uint8_t)*rec_data->NumDMXOutputs);
	data += rec_data->NumDMXOutputs;

	rec_data->DmxOutUniv = new uint16_t[rec_data->NumDMXOutputs];
	memcpy(rec_data->DmxOutUniv, data, sizeof(uint16_t)*rec_data->NumDMXOutputs);
	data += (rec_data->NumDMXOutputs * 2);

	for (int i(0); i < (int)rec_data->NumDMXOutputs; i++) {
		bswap_16(rec_data->DmxOutUniv[i]);
	}

	memcpy(&rec_data->NumDrivers, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->DriverNameLength, data, sizeof(uint8_t));
	data += 1;

	rec_data->DriverType = new uint8_t[rec_data->NumDrivers];
	memcpy(rec_data->DriverType, data, sizeof(uint8_t)*rec_data->NumDrivers);
	data += rec_data->NumDrivers;

	rec_data->DriverSpeed = new uint8_t[rec_data->NumDrivers];
	memcpy(rec_data->DriverSpeed, data, sizeof(uint8_t)*rec_data->NumDrivers);
	data += rec_data->NumDrivers;

	rec_data->DriverExpandable = new uint8_t[rec_data->NumDrivers];
	memcpy(rec_data->DriverExpandable, data, sizeof(uint8_t)*rec_data->NumDrivers);
	data += rec_data->NumDrivers;

	rec_data->DriverNames = new char*[rec_data->NumDrivers];

	for (int i = 0; i < rec_data->NumDrivers; i++) {
		rec_data->DriverNames[i] = new char[rec_data->DriverNameLength + 1];
		memset(rec_data->DriverNames[i], 0, sizeof(uint8_t)*rec_data->DriverNameLength + 1);
		memcpy(rec_data->DriverNames[i], data, sizeof(uint8_t)*rec_data->DriverNameLength);
		data += rec_data->DriverNameLength;
	}

	memcpy(&rec_data->CurrentDriver, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->CurrentDriverType, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->CurrentDriverSpeed, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->CurrentDriverExpanded, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->Gamma, data, sizeof(uint8_t) * 4);
	data += 4;

	rec_data->Gammaf[0] = (float)rec_data->Gamma[0] * 0.1;
	rec_data->Gammaf[1] = (float)rec_data->Gamma[1] * 0.1;
	rec_data->Gammaf[2] = (float)rec_data->Gamma[2] * 0.1;
	rec_data->Gammaf[3] = (float)rec_data->Gamma[3] * 0.1;

	memcpy(rec_data->Nickname, data, sizeof(uint8_t) * 40);
	data += 40;

	memcpy(&rec_data->Temperature, data, sizeof(uint16_t));
	data += 2;
	bswap_16(rec_data->Temperature);

	memcpy(&rec_data->MaxTargetTemp, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->NumBanks, data, sizeof(uint8_t));
	data += 1;

	rec_data->VoltageBanks = new uint16_t[rec_data->NumBanks];
	memcpy(rec_data->VoltageBanks, data, sizeof(uint16_t)*rec_data->NumBanks);
	data += (rec_data->NumBanks * 2);

	for (int i(0); i < (int)rec_data->NumBanks; i++) {
		bswap_16(rec_data->VoltageBanks[i]);
	}

	memcpy(&rec_data->TestMode, data, sizeof(uint8_t));
	data += 1;

	memcpy(rec_data->TestCols, data, sizeof(uint8_t) * 4);
	data += 4;

	memcpy(&rec_data->TestOutputNum, data, sizeof(uint8_t));
	data += 1;

	memcpy(&rec_data->TestPixelNum, data, sizeof(uint16_t));
	data += 2;
	bswap_16(rec_data->TestPixelNum);

	if (!deviceExist(rec_data->Mac)) advatek_manager::devices.emplace_back(rec_data);

}

void advatek_manager::process_opTestAnnounce(uint8_t * data) {
	return;
}

void advatek_manager::process_udp_message(uint8_t * data) {
	char ID[9];
	memcpy(ID, data, sizeof(uint8_t) * 9);
	data += 9;
	std::cout << "ID: " << ID << std::endl;

	std::string sid;
	for (int i(0); i < 8; i++) { sid += ID[i]; }
	if (sid.compare("Advatech") != 0) return; // Not an Advatek message ...

	//--------

	uint16_t OpCodes;
	memcpy(&OpCodes, data, sizeof(uint16_t));
	data += 2;
	// Swap bytes
	bswap_16(OpCodes);
	std::cout << "messageType: " << OpCodes << std::endl;

	switch (OpCodes) {
	case OpPollReply:
		process_opPollReply(data);
		return;
	case OpTestAnnounce:
		process_opTestAnnounce(data);
		return;
	default:
		return;
	}
}

void advatek_manager::refreshAdaptors() {

	advatek_manager::networkAdaptors.clear();

	boost::asio::ip::tcp::resolver::iterator it;
	try
	{
		it = resolver.resolve(query);
	}
	catch (boost::exception& e)
	{
		std::cout << "Query didn't resolve Any adaptors - " << boost::diagnostic_information(e) << std::endl;
		return;
	}

	while (it != boost::asio::ip::tcp::resolver::iterator())
	{
		boost::asio::ip::address addr = (it++)->endpoint().address();
		std::cout << "adaptor found: " << addr.to_string() << std::endl;
		if (addr.is_v4()) {
			advatek_manager::networkAdaptors.push_back(addr.to_string());
		}
	}
	if (advatek_manager::networkAdaptors.size() > 0) {
		currentAdaptor = 0;
	}
}

std::string advatek_manager::macStr(uint8_t * address)
{
	std::stringstream ss;

	for (int i(0); i < 6; i++) {
		ss << std::hex << std::setw(2) << static_cast<int>(address[i]);
		if (i < 5) ss << ":";
	}

	return ss.str();
}

std::string advatek_manager::ipStr(uint8_t * address)
{
	std::stringstream ss;

	for (int i(0); i < 4; i++) {
		ss << static_cast<int>(address[i]);
		if (i < 3) ss << ".";
	}

	return ss.str();
}
