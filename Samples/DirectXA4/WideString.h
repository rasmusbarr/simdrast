//
//  WideString.h
//  DirectXA4
//
//  Created by Rasmus Barringer on 2013-06-07.
//  Copyright (c) 2013 Rasmus Barringer. All rights reserved.
//

#ifndef DirectXA4_WideString_h
#define DirectXA4_WideString_h

#include <vector>
#include <string>

class WideString {
private:
	std::vector<wchar_t> v;

public:
	WideString(const char* str);

	WideString(const std::string& str);

	operator wchar_t* ();
};

#endif
