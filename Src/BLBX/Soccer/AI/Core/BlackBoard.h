#pragma once
#include <variant>
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <DirectXMath.h>
#include <Windows.h>

using BBValue = std::variant<int, float, bool, std::string, DirectX::XMFLOAT3>;
class BlackBoard
{
private:
	std::unordered_map<std::string, BBValue> data;
	BlackBoard* parent = nullptr;

public:
	BlackBoard(BlackBoard* p = nullptr) : parent(p) {}

	void Set(const std::string& key, BBValue value)
	{
		data[key] = value;
	}

	template<typename T>
	std::optional<T> Get(const std::string& key) const
	{
        auto it = data.find(key);

        if (it != data.end()) 
		{
			if (const T* value = std::get_if<T>(&it->second))
			{
				return *value;
			}

			// キーは存在するが、要求した型と違う
#ifdef _DEBUG
			const std::string message =
				"[BlackBoard] Type mismatch: " + key + "\n";

			OutputDebugStringA(message.c_str());
#endif
			return std::nullopt;
        }

        if (parent) 
		{
            return parent->Get<T>(key);
        }
		return std::nullopt;
    }

	template<typename T>
	std::optional<T> GetAndRemove(const std::string& key)
	{
		auto it = data.find(key);

		if (it != data.end())
		{
			const T* value = std::get_if<T>(&it->second);

			if (!value)
			{
#ifdef _DEBUG
				const std::string message =
					"[BlackBoard::GetAndRemove] Type mismatch: " + key + "\n";

				OutputDebugStringA(message.c_str());
#endif
				return std::nullopt;
			}

			T result = *value;
			data.erase(it);
			return result;
		}

		if (parent)
		{
			return parent->GetAndRemove<T>(key);
		}

		return std::nullopt;
	}

	bool Remove(const std::string& key)
	{
		auto it = data.find(key);
		if (it != data.end())
		{
			data.erase(it);
			return true;
		}
		if (parent)
		{
			return parent->Remove(key);
		}
		return false;
	}

	bool Has(const std::string& key) const
	{
		if (data.find(key) != data.end())
		{
			return true;
		}

		if (parent)
		{
			return parent->Has(key);
		}
		return false;
	}

	void SetParent(BlackBoard* p) { parent = p; }

	BlackBoard* GetParent()const { return parent; }
};