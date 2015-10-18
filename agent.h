#pragma once

#include "position.h"

class Agent
{
	Position* m_position;
	//Velocity m_velocity;
	float m_lookSpeed;

	public:

	Agent(const Position& position, float lookSpeed);
};

void sendInputToAgent();
