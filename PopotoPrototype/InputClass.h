#pragma once
// ----------------------------
// ----Class definition----
// ----------------------------
class InputClass {
public:
	InputClass() {}

	void KeyDown(UINT8 val) { 
		m_keys[val] = true; 
	}

	void KeyUp(UINT8 val) {
		m_keys[val] = false; 
	}

	bool IsKeyDown(UINT8 val) const {
		return m_keys[val]; 
	}

	// Delete functions
	InputClass(InputClass const& rhs) = delete;
	InputClass& operator=(InputClass const& rhs) = delete;

	InputClass(InputClass&& rhs) = delete;
	InputClass& operator=(InputClass&& rhs) = delete;

	enum keyCodes {

	};

private:
	std::array<bool, 256> m_keys{};
};