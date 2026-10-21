#pragma once
class Debug
{
public:

#ifdef NODEBUG
	static constexpr bool EnableDebug = false;

#else

	static constexpr bool EnableDebug = true;

#endif // 

};

