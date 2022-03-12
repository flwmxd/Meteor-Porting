//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>
#include <map>
#include <cstdint>
#include <memory>

namespace meteor 
{
	template <typename T>
	class Cache
	{
	public:
		virtual ~Cache()
		{
		}

		static auto get(const std::string & name) -> T&
		{
			auto iter = cache.find(name);
			if (iter == cache.end())
			{
				iter = cache.emplace(name, T{ name })
					.first;
			}
			return iter->second;
		}

		static auto remove(const std::string& id)
		{
			cache.erase(id);
		}

		static auto clear() -> void
		{
			cache.clear();
		}

	private:
#ifdef __GNUC__ 
		static std::map<std::string, T> cache;
#else
		static std::unordered_map<std::string, T> cache;
#endif
	};
	template <typename T>
#ifdef __GNUC__ 
	std::map<std::string, T> Cache<T>::cache;
#else
	std::unordered_map<std::string, T> Cache<T>::cache;
#endif
}

