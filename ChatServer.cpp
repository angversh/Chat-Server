#include <iostream>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <map>

using namespace std;
using json = nlohmann::json;

const string COMMAND = "command";
const string PRIVATE_MSG = "privateMsg";
const string PUBLIC_MSG = "publicMsg";
const string MESSAGE = "message";

const string USER_ID = "userId";
const string SET_NAME = "setName";
const string NEW_NAME = "newName";

const string USER_ID_TO = "userIdto";
const string USER_ID_FROM = "userIdfrom";

const string STATUS = "status";
const string ONLINE = "online";
const string PUBLIC_ALL = "publicAll";

struct UserData {
	int userId; 
	string name;
};

map<int, UserData*> all_users;

typedef uWS::WebSocket<false, true, UserData> UWEB;

void processPrivateMsg(json parsed, UWEB *ws, int userId){
	int userIdto = parsed[USER_ID]; // server recieves a message
	string userMsg = parsed[MESSAGE];

	//server sends a message 
	json response;
	response[COMMAND] = PRIVATE_MSG;
	response[USER_ID_FROM] = userId;
	response[MESSAGE] = userMsg;

	ws->publish("Unknown" + to_string(userIdto), response.dump());
}

void processPublicMsg(json parsed, UWEB* ws, int userId) {
	string userMsg = parsed[MESSAGE];

	json response;
	response[COMMAND] = PRIVATE_MSG;
	response[USER_ID_FROM] = userId;
	response[MESSAGE] = userMsg;

	ws->publish(PUBLIC_ALL, response.dump());
}

void processSetName(json parsed, UWEB* ws, UserData* data) {
	data->name = parsed[NEW_NAME];
}

void processMessage(UWEB *ws, string_view message) {
	UserData* data = ws->getUserData();

	cout << "Message from user ID: " << data->userId << " message " << message << endl;
	auto parsed = json::parse(message);

	if (parsed[COMMAND] == PRIVATE_MSG) {
		processPrivateMsg(parsed, ws, data->userId);
	}
	if (parsed[COMMAND] == PUBLIC_MSG) {
		processPublicMsg(parsed, ws, data->userId);
	}
	if (parsed[COMMAND] == SET_NAME) {
		processSetName(parsed, ws, data);
		ws->publish(PUBLIC_ALL, status(data, true));
	}
}

//returns json with the status
string status(UserData* data, bool online) {
	json response;
	response[COMMAND] = STATUS;
	response[NEW_NAME] = data->name;
	response[USER_ID] = data->userId;
	response[ONLINE] = online;
	return response.dump();
}

int main()
{
	int initialId = 10;

	uWS::App().ws<UserData>("/*", {
		.idleTimeout = 1000,
		.open = [&initialId](auto* ws) { 
			UserData *data = ws->getUserData(); 
			data->userId = initialId++;
			data->name = "Unnamed";

			cout << "New user connected ID: " << data->userId << endl;
			ws->subscribe("Unknown" + to_string(data->userId));
			ws->publish(PUBLIC_ALL, status(data, true));
			ws->subscribe(PUBLIC_ALL);

			for (auto entry : all_users) { //informing about other users
				ws->send(status(entry.second, true),uWS::OpCode::TEXT);
			}

			all_users[data->userId] = data;
		},

		.message = [](auto* ws, string_view message, uWS::OpCode) {
			processMessage(ws, message);
		},

		.close = [](auto *ws, int code, string_view message) { //3 FUNCTION: текст об откл. пользователя;
			UserData* data = ws->getUserData();

			cout << "User disconnected ID : " << data->userId << endl;
			ws->publish(PUBLIC_ALL, status(data, false));

			all_users.erase(data->userId);
		},
		}).listen(9001, [](auto*) {
			cout << "Hi! Server Started Successfully!" << endl;
		}).run();
}