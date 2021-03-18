// SkillChatServer.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <uwebsockets/App.h>
#include <algorithm>
#include <string>
#include <map>
#include <regex>
using namespace std;

/* We simply call the root header file "App.h", giving you uWS::App and uWS::SSLApp */


/* This is a simple WebSocket echo server example.
 * You may compile it with "WITH_OPENSSL=1 make" or with "make" */

map<unsigned int, string> userNames;

struct PerSocketData {
    string name;
    unsigned int user_id;
};

const string BROADCAST_CHANNEL = "broadcast";
const string MESSAGE_TO = "MESSAGE_TO::";
const string SET_NAME = "SET_NAME::";
const string ONLINE = "ONLINE::";
const string OFFLINE = "OFFLINE::";

string online(unsigned int user_id) {
    string name = userNames[user_id];
    return ONLINE + to_string(user_id)+ "::"+ name;
}

string offline(unsigned int user_id) {
    string name = userNames[user_id];
    return OFFLINE + to_string(user_id) + "::" + name;
}

const map<string, string> DataBase = {
        {"hello","Hi, how are you?" },
        {"how are you","I'm fine"},
        {"exit","See you soon"},
        {"weather","There's no bad weather"},
        {"can","Only speaking"},
};
string toLower(string txt) {
    transform(txt.begin(), txt.end(), txt.begin(), ::tolower);
    return txt;
}

void updateName(PerSocketData* data) {
    userNames[data->user_id] = data->name;
}

//string userAsk() {
//    string question;
//    cout << "[USER:] ";
//    getline(cin, question);
//    question = toLower(question);
//    return question;
//}

void deleteName(PerSocketData* data) {
    userNames.erase(data->user_id);
}

bool isSetName(string message) {
    return message.find(SET_NAME) == 0;
}

string parseName(string message) {
    return message.substr(SET_NAME.size());
}

bool isMessageTo(string message) {
    return message.find("MESSAGE_TO::") == 0;
}

string parseUserId(string message) {
    string rest = message.substr(MESSAGE_TO.size()); /*11::привет*/
    int pos = rest.find("::"); /*pos=2*/
    return rest.substr(0, pos); /*11*/
}

string parseUserText(string message) {
    string rest = message.substr(MESSAGE_TO.size()); /*11::привет*/
    int pos = rest.find("::"); /*pos=2*/
    return rest.substr(pos+2); /*Привет*/
}

string messageFrom(string user_id, string senderName, string message) {
    return "MESSAGE FROM::" + user_id + "::"+"["+senderName+"]"+ message;
}

int main() {
    /* инфа о пользователе */
    
    //PerSocketData Bot{
    //    .name="BOT", .user_id=1,
    //};

    unsigned int last_user_id = 10; /*последний ид польз*/
    unsigned int count_users = 0;

    uWS::App(). /*Создаем сервер*/
        ws<PerSocketData>("/*", { /*Структура данные пользователя в виде структуры*/
            /* Settings */
            .idleTimeout = 1200, /*колво секунд до отключения афк*/
            /* Handlers */
            .open = [&last_user_id,&count_users](auto* ws) {
                /* лямбда функция при открытии соединения */
                PerSocketData* userData = (PerSocketData *) ws->getUserData();
                userData->name = "UNNAMED";
                userData->user_id = last_user_id++;
                for (auto entry : userNames) {
                    ws->send(online(entry.first), uWS::OpCode::TEXT);
                }
                updateName(userData);
                ws->publish(BROADCAST_CHANNEL,online(userData->user_id));
                count_users++;
                cout << "New user connected,id= " << userData->user_id << " Total users number: " << count_users << endl;
                string user_channel = "user#" + to_string(userData->user_id);
                ws->subscribe(user_channel);
                ws->subscribe(BROADCAST_CHANNEL);
            },
            .message = [&last_user_id](auto* ws, std::string_view message, uWS::OpCode opCode) {
                string strMessage = string(message);
                PerSocketData* userData = (PerSocketData*)ws->getUserData();
                string authorId = to_string(userData->user_id);
                if (isMessageTo(strMessage)) {
                    string receiverId = parseUserId(strMessage);
                    string text = parseUserText(strMessage);
                    string outgoingMessage = messageFrom(authorId, userData->name, text);
                    if (stoi(receiverId) > last_user_id) {
                        cout << "Author #" << authorId << " try to send a message to undefined user "<< endl;
                    }
                    else {
                        ws->publish("user#" + receiverId, outgoingMessage, uWS::OpCode::TEXT, false);
                        cout << "Author #" << authorId << " wrote message to " << receiverId << endl;
                    }
                    if (stoi(receiverId) == 1) {
                        int numAnswer = 0;
                            for (auto word : DataBase) {
                                regex key = regex(".*" + word.first + ".*");
                                if (regex_match(toLower(outgoingMessage), key)) {
                                    numAnswer++;
                                    ws->send("[BOT]" + word.second, uWS::OpCode::TEXT);
                                }
                            }
                            if (numAnswer == 0) {
                                ws->send("[BOT] You can ask me something ", uWS::OpCode::TEXT);
                            }
                            if (numAnswer == 4) {
                                ws->send("[BOT] Uh. I'm so tired today ", uWS::OpCode::TEXT);

                            }
                    }
                    
}
                if (isSetName(strMessage)) {
                    string newName = parseName(strMessage);
                    if (newName.find("::") != std::string::npos || newName.size()>=255) {
                        cout << "User #" << authorId << "Sets wrong name" << endl;
                    }
                    else {
                        userData->name = newName;
                        updateName(userData);
                        ws->publish(BROADCAST_CHANNEL, online(userData->user_id));
                        cout << "User #" << authorId << " set his name" << endl;
                    }
                    /*userData->name = newName;
                    cout << "User #" << authorId << "Set their name" << endl;*/
                }
            },
            .close = [](auto*ws, int /*code*/, std::string_view /*message*/) {
                /* откл от сервера */
                PerSocketData* userData = (PerSocketData*)ws->getUserData();
                ws->publish(BROADCAST_CHANNEL, offline(userData->user_id));
                deleteName(userData);
            }
            }).listen(9001, [](auto* listen_socket) {
                if (listen_socket) {
                    std::cout << "Listening on port " << 9001 << std::endl;
                }
                }).run();
}
