#include "world_clock.h"
#include "globals.h"

WorldClock::WorldClock(double multiplier, double t) :
	m_multiplier(multiplier), m_dt(t), m_t(t)
{
	TwAddVarRW(GLOBALS.m_overlay_bar, "Multiplier", TW_TYPE_DOUBLE, &m_multiplier, "step=1 group=WorldClock ");
	TwAddVarRO(GLOBALS.m_overlay_bar, "Time", TW_TYPE_DOUBLE, &m_t, " group=WorldClock ");
}
