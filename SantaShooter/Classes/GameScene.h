#pragma once

#include <vector>
#include <deque>
#include <memory>
#include <chrono>
#include <string>
#include <sstream>

#include "cocos2d.h"
#include "cocostudio/CocoStudio.h"
#include "ui/CocosGUI.h"
#include "network/WebSocket.h"

#include "../external/json/document.h"
#include "../external/json/writer.h"
#include "../external/json/stringbuffer.h"

#include "PlayerCharacter.h"

#include <time.h>

enum class Role
{
	UNINITIALIZED = 0,
	SERVER,
	CLIENT1,
	CLIENT2
};

enum class Opcode
{
	HELLO = 0,
	PING = 1,
	PONG = 2,
	WORLD_STATE = 3,
};

class GameScene : public cocos2d::Node, public cocos2d::network::WebSocket::Delegate
{
private:
	int score1 = 0;
	int score2 = 0;

	cocos2d::network::WebSocket* websocket = nullptr;

	Role role = Role::UNINITIALIZED;
	std::deque<std::string> consoleLines;
	std::chrono::high_resolution_clock::time_point pingStartTime;
	long long pingTime;

	void setupPlayers();
	void updateStatus();
	void updateScore();

	template<class T>
	int addKV(const T& value, rapidjson::Document& json, std::deque<std::string>& keys)
	{
		int n = 0;
		std::stringstream ss;
		ss << "a" << keys.size();
		keys.push_back(ss.str());

		// While iterators can be invalidated on insert into deque, references are not
		json.AddMember(keys.back().c_str(), value, json.GetAllocator());
		return n;
	}

	template<class... Ts>
	std::string createMessage(Opcode opcode, Role target, Ts... args)
	{
		rapidjson::Document json;
		json.SetObject();

		json.AddMember("o", (int)opcode, json.GetAllocator());
		json.AddMember("d", (int)target, json.GetAllocator());

		// Initializer list with always more than 0 elements.
		// In a compound statement within parentheses, the right-most value returns while evaluating left to right
		std::deque<std::string> keys;
		using tempAlias = int[];
		tempAlias{ 0, (addKV(args, json, keys), 0)... };

		rapidjson::StringBuffer sb;
		rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
		json.Accept(writer);

		return std::string(sb.GetString());
	}

	void addConsoleText(std::string text);

	void send(const std::string& message);
	void sendPing();
	void sendPong(Role target);

public:
    // there's no 'id' in cpp, so we recommend returning the class instance pointer
    static cocos2d::Scene* createScene();

    // Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
    virtual bool init();
    
	virtual void update(float deltaTime);

    // a selector callback
    void menuCloseCallback(cocos2d::Ref* pSender);
    
    // implement the "static create()" method manually
    CREATE_FUNC(GameScene);

	virtual void onOpen(cocos2d::network::WebSocket* ws);
	virtual void onMessage(cocos2d::network::WebSocket* ws, const cocos2d::network::WebSocket::Data& data);
	virtual void onClose(cocos2d::network::WebSocket* ws);
	virtual void onError(cocos2d::network::WebSocket* ws, const cocos2d::network::WebSocket::ErrorCode& error);

	void onTouchesBegan(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event);
	void onTouchesEnded(const std::vector<cocos2d::Touch*>& touches, cocos2d::Event* event);
	bool onContactBegin(const cocos2d::PhysicsContact& contact);
	void onConnectButtonPressed(Ref* pSender, cocos2d::ui::Widget::TouchEventType type);

	CC_SYNTHESIZE(PlayerCharacter*, player1, Player1);
	CC_SYNTHESIZE(PlayerCharacter*, player2, Player2);
};

