#pragma once
#include <string>

namespace REON {
	struct ResourceInfo
	{
		std::string UID; //original asset uid
		std::string path; //original asset path
		int localIdentifier; //unique identifier for subobject within asset
	};
}