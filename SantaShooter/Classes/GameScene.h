#pragma once

#include <vector>
#include <deque>
#include <algorithm>
#include <tuple>
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

static const int CONSOLE_LOG_LINES = 10;

static const int CLIENT_ACTION_LOG_CAPACITY = 360;
static const int SERVER_POSITION_LOG_CAPACITY = 180;

static const float REWIND_DISTANCE_THRESHOLD = 0.5f;

class GameScene : public cocos2d::Node, public cocos2d::network::WebSocket::Delegate
{
private:
	enum class Role
	{
		UNINITIALIZED = 0,
		SERVER,
		CLIENT1,
		CLIENT2,
		ALL_CLIENTS,
	};

	enum class Opcode
	{
		HELLO = 0,
		PING = 1,
		PONG = 2,
		HANDSHAKE_ACK = 3,
		WORLD_STATE = 4,
		KEY_INPUT = 5,
		FIRE = 6,
		REMOVE_GIFTBOX = 7,
	};

	bool bailout = false;

	cocos2d::network::WebSocket* websocket = nullptr;

	Role role = Role::UNINITIALIZED;

	std::deque<std::tuple<int64_t, KeyInput, cocos2d::Point, cocos2d::Point>> clientActionLog;

	std::deque<std::string> consoleLines;

	std::chrono::high_resolution_clock::time_point gameStartTime;
	std::chrono::high_resolution_clock::time_point pingStartTime;

	int64_t lastMessageCreatedTimestamp = 0;
	int64_t tickSequence = 0;

	inline int64_t getCurrentTimestamp()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - gameStartTime).count();
	}

	void setupPlayers();
	void updateStatus();
	void updateScore();

	template<class T>
	void addKV(const T& value, rapidjson::Document& json, std::deque<std::string>& keys)
	{
		std::stringstream ss;
		ss << "a" << keys.size();
		keys.push_back(ss.str());

		// While iterators can be invalidated on insert into deque, references are not
		json.AddMember(keys.back().c_str(), value, json.GetAllocator());
	}

	template<class... Ts>
	std::string createMessage(Opcode opcode, int64_t lastAckTickSequence, Role target, Ts... args)
	{
		lastMessageCreatedTimestamp = getCurrentTimestamp();

		rapidjson::Document json;
		json.SetObject();

		json.AddMember("o", (int)opcode, json.GetAllocator());
		json.AddMember("t", tickSequence, json.GetAllocator());
		json.AddMember("a", lastAckTickSequence, json.GetAllocator());
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

	// Client message actions
	void sendPing();
	void sendHandshakeAck();
	void sendKeyInput(Role origin, KeyInput keyInput);
	void sendFire(Role origin, cocos2d::Point point);

	// Server message actions
	void sendPong(Role target, int64_t ackTickSequence);
	void sendWorldState(Role target, int64_t ackTickSequence);
	void sendRemoveGiftbox(Role target, int64_t ackTickSequence, int64_t giftboxId);

	void addClientActionLog(PlayerCharacter* player);
	void acceptAuthoritativeWorldState(
		int64_t lastAckTickSequence,
		cocos2d::Point player1Position, cocos2d::Point player1Velocity, int player1Score,
		cocos2d::Point player2Position, cocos2d::Point player2Velocity, int player2Score
	);
	void rewindAndReplayClientWorldState(
		PlayerCharacter* player, cocos2d::Point authoritativePlayerPosition, cocos2d::Point authoritativePlayerVelocity, int64_t lastAckTickSequence);
	cocos2d::Sprite* createLagCompensationHitbox(PlayerCharacter* player, PlayerCharacter* opponent);

public:
	static cocos2d::Scene* createScene();
	virtual bool init();

	virtual void update(float deltaTime);

	void menuCloseCallback(cocos2d::Ref* pSender);

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

	CC_SYNTHESIZE(int64_t, lastAckTickFromServer, LastAckTickFromServer);
	CC_SYNTHESIZE(int64_t, pingTime, PingTime);

	void saveGiftboxesProperties();
	void restoreGiftboxesProperties();
};

