#pragma once
#ifndef UISTUFF_H
#define UISTUFF_H

#include <string>

namespace UIStuff {


	std::string OpenFolderDialog();
	std::string OpenFileDialog(const std::wstring& filter = L"All Files\0*.*\0");

	class PopUp {
	public:
		PopUp() : popUpMessage("message"), popUpLabel("label") {}
		~PopUp() { HidePopUp(); }

		void ShowPopUp(const std::string& label, const std::string& message);
		void HidePopUp();
		void RenderPopUp();
	private:
		bool isPopUpShowing = false;
		std::string popUpMessage;
		std::string popUpLabel;
	};

	

}
#endif // UISTUFF_H
