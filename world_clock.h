#pragma once

class WorldClock
{
	double m_multiplier;
	double m_dt;
	double m_t;

public:

	WorldClock(double multiplier, double t);

	inline double getMultiplier() const { return m_multiplier; }
	inline double getDt() const { return m_dt; }
	inline double getT() const { return m_t; }

	inline void setMultiplier(double multiplier) { m_multiplier = multiplier; }

	inline void updateFromSystemClock(double dt)
	{
		m_dt = m_multiplier * dt;
		m_t += m_dt;
	}
};
