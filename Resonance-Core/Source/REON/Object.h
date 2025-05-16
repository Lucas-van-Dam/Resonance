#pragma once

#include "string"
#include <rpc.h>
#include <Windows.h>
#include "REON/Logger.h"

namespace REON {

	class Object
	{
	public:
		Object(const std::string& name = "Unnamed Object") : m_ID(GenerateUUID()), m_Name(name) {}
		virtual ~Object() = default;

		std::string GetID() const { return m_ID; }
		/// <summary>
		/// DO NOT USE UNLESS YOU ARE CONSTRUCTING AN OBJECT FROM A FILE, THIS WILL BREAK CONNECTIONS WITH RESOURCE MANAGER AND POTENTIALLY OTHER SYSTEMS
		/// </summary>
		/// <param name="id"></param>
		//void SetID(std::string id) { m_ID = id; }

		const std::string& GetName() const { return m_Name; }
		virtual std::string ToString() const { return m_Name + " [" + m_ID + "]"; }

		void SetName(const std::string& name) { m_Name = name; }

	protected:
		std::string m_ID;
		std::string m_Name;

	private:
		static std::string GenerateUUID() {
			UUID uuid;
			RPC_STATUS status = UuidCreate(&uuid);

			if (status == RPC_S_OK) {
				std::string uuidStringOut;
				RPC_CSTR uuidString;
				UuidToStringA(&uuid, &uuidString);
				uuidStringOut = (char*)uuidString;
				RpcStringFreeA(&uuidString);
				return uuidStringOut;
			}
			else {
				REON_CORE_ERROR("Failed to create UUID. Error code: {}", status);
			}

			return 0;

		}
	};

}

