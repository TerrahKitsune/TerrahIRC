#include "KitsuneSwagConsole.h"

BOOL CALLBACK OptionsWindowProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:

		SetWindowText(GetDlgItem(hwnd, IDC_START), ASWN->StartScript);
		SetWindowText(GetDlgItem(hwnd, IDC_RECV), ASWN->RecvFunc);
		SetWindowText(GetDlgItem(hwnd, IDC_SEND), ASWN->SendFunc);
		SetWindowText(GetDlgItem(hwnd, IDC_TICK), ASWN->TickFunc);
		SetWindowText(GetDlgItem(hwnd, IDC_LUAEXIT), ASWN->ExitFunc);
		SetWindowText(GetDlgItem(hwnd, IDC_LUAERROR), ASWN->ErrorFunc);

		char buf[20];
		sprintf(buf, "%d", ASWN->ScreenSplit);
		SetWindowText(GetDlgItem(hwnd, IDC_SCREENSPL), buf);

		sprintf(buf, "%d", ASWN->MsgLoopTimer);
		SetWindowText(GetDlgItem(hwnd, IDC_MGSLOOPT), buf);
		
		sprintf(buf, "%d", ASWN->MilisecondTicks);
		SetWindowText(GetDlgItem(hwnd, IDC_LUAHB), buf);

		sprintf(buf, "%d", ASWN->MaxConsoleLetters);
		SetWindowText(GetDlgItem(hwnd, IDC_CMCF), buf);

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:{

					  char * n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_START));

					  if (n){
						  delete[]ASWN->StartScript;
						  ASWN->StartScript = n;
					  }

					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_RECV));

					  if (n){
						  delete[]ASWN->RecvFunc;
						  ASWN->RecvFunc = n;
					  }

					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_SEND));

					  if (n){
						  delete[]ASWN->SendFunc;
						  ASWN->SendFunc = n;
					  }

					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_TICK));

					  if (n){
						  delete[]ASWN->TickFunc;
						  ASWN->TickFunc = n;
					  }

					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_LUAEXIT));

					  if (n){
						  delete[]ASWN->ExitFunc;
						  ASWN->ExitFunc = n;
					  }

					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_LUAERROR));

					  if (n){
						  delete[]ASWN->ErrorFunc;
						  ASWN->ErrorFunc = n;
					  }

					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_SCREENSPL));

					  if (n){
						  sscanf(n, "%d", &ASWN->ScreenSplit);
						  delete[]n;
					  }

					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_MGSLOOPT));
					  
					  if (n){
						  sscanf(n, "%d", &ASWN->MsgLoopTimer);
						  delete[]n;
					  }
					  
					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_LUAHB));

					  if (n){
						  sscanf(n, "%d", &ASWN->MilisecondTicks);
						  delete[]n;
					  }

					  n = ASWN->GetWindowTextGoodEdition(GetDlgItem(hwnd, IDC_CMCF));

					  if (n){
						  sscanf(n, "%d", &ASWN->MaxConsoleLetters);
						  delete[]n;
					  }

					  ASWN->SaveAllShit();

					  ASWN->TC->TypeSetSize = ASWN->ScreenSplit;

					  ASWN->TC->TextMax = ASWN->MaxConsoleLetters;
					  SendMessage(ASWN->TC->hwndOutput, EM_SETLIMITTEXT, (ASWN->TC->TextMax - 1) / 2, 0);

					  ASWN->TC->HandleResize(WM_SIZE);

					  //KillTimer(NULL, ASWN->timerId);

					  /*if (ASWN->MsgLoopTimer <= 0)
						  ASWN->timerId = SetTimer(ASWN->TC->hwnd, NULL, USER_TIMER_MINIMUM, NULL);
					  else
						  ASWN->timerId = SetTimer(ASWN->TC->hwnd, NULL, ASWN->MsgLoopTimer, NULL);*/

					  EndDialog(hwnd, IDOK);
					  break;
		}
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ASWN->TC->HandleResize(msg);
	switch (msg)
	{

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case ID_GTFO:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case ID_SUPERBUTTON:
			//memset(ASWN->TC->consolebuffer, 0, ASWN->TC->consolebuffersize);
			SetWindowText(ASWN->TC->hwndOutput, "");
			break;

		case ID_SHITBUTTON:
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);

			break;
		case ID_SWAG_LUAOPTIONS:

			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), hwnd, OptionsWindowProc);
			break;
		case ID_SWAG_RUNLUASCRIPT:{

									  ASWN->ExhangeMutex = 3;
		}
			break;
		}


		break;

	case WM_LBUTTONDOWN:



		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
	{

	}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK subEditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYUP:{

					  if (wParam == VK_RETURN){
						  if (ASWN->EnterIsDown){
							  ASWN->EnterIsDown = false;
							  SetWindowText(ASWN->TC->hwndInput, "");
							  return 0;
						  }
					  }
	}
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RETURN:{				   

						   if (GetKeyState(VK_SHIFT) & 0x8000){
							   return 0;
						   }

						   if (ASWN->EnterIsDown)
							   return 0;
						   else
							   ASWN->EnterIsDown = true;

						   int sbsize = GetWindowTextLength(ASWN->TC->hwndInput) + 3;
						   char * buf = new char[sbsize];

						   if (ASWN->prevlua)
							   delete[]ASWN->prevlua;
						   ASWN->prevlua = buf;

						   if (!buf){

							   MessageBox(NULL, "Memory issues with inputbox!", "Lame!", NULL);
							   return 0;
						   }

						   ASWN->TC->Gets(buf, sbsize);
						
						   if (buf[0] != 0){

							   if (buf[0] == '/' &&
								   buf[1] == 'L' &&
								   buf[2] == 'U' &&
								   buf[3] == 'A' &&
								   buf[4] == ' '){

								   ASWN->MainToLuaThreadCall(NULL, &buf[5], sbsize-8);
							   }
							   else if (ASWN->SendFunc && !ASWN->MainToLuaThreadCall(ASWN->SendFunc, buf, sbsize-3)){
								   strcat(buf, "\r\n");
								   ASWN->TC->Puts(buf,strlen(buf));
							   }
							   else if(!ASWN->SendFunc || ASWN->SendFunc[0]=='\0'){
								   
								   strcat(buf, "\r\n");
								   ASWN->TC->Puts(buf, strlen(buf));
							   }
						   }
						   return 0;
		}
		}
	default:
		return CallWindowProc(ASWN->TC->InputDefProc, wnd, msg, wParam, lParam);
	}
	return 0;
}