#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "TitanAddOn.h" 
#include "Offsets.h" 
#include "Headers/Map.h" 
#include "GUIConnect.h"
#include "Headers/AntiHack.h"  

char *mainVersion   = (char*)(VERSION_IN_MAIN); 
char *mainSerial    = (char*)(SERIAL_IN_MAIN); 

Config config;

static unsigned char bBuxCode[3]={0xAB, 0xDC, 0xEF};	// Xox Key for some interesthing things :)
void BuxConvert(char* buf, int size)
{
	int n;

	for (n=0;n<size;n++)
	{
		buf[n]^=bBuxCode[n%3] ;		// Nice trick from WebZen
	}
}

char* GetTokenString(char * str)
{
	char seps[2] = " ";
	return strtok(str, seps);
}


int GetTokenNumber(char * str)
{
	char seps[2] = " ";
	char * szToken = strtok(str, seps);

	if ( szToken != NULL )
	{
		return atoi(szToken);
	}

	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			hInstance = hModule;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

void PHeadSetB(LPBYTE lpBuf, BYTE head, int size)
{
	lpBuf[0] =0xC6;		// Packets
	lpBuf[1] =size;
	lpBuf[2] =head;
}
void PHeadSubSetB(LPBYTE lpBuf, BYTE head, BYTE sub, int size)
{
	lpBuf[0] =0xC6;	// Packets
	lpBuf[1] =size;
	lpBuf[2] =head;
	lpBuf[3] =sub;
}

bool FileExists(char * name)
{
	if(CreateFile(name,0,0,0,OPEN_EXISTING,0,0) == INVALID_HANDLE_VALUE)
		return false;
	return true;
}
void ReplaceBytes(DWORD Address, BYTE ReplaceTo, int HowMany)
{
	for(int i=0;i<HowMany;i++)
		*(BYTE*)(Address+i) = ReplaceTo;
}
__declspec() void HookForMiniMap();

void ProtocolCore(unsigned char protoNum,unsigned char* aRecv,int aLen,int aIndex)
{
	switch(protoNum)
	{
		//case 0xD3:
		case 0xE0:
		{
			if(aRecv[3] = 0x99)
			{
				DataSend(aRecv,aLen);
			}
		};
		case 0xF1:
		case 0xF3:
		{
			gui.SEND_VShopClose((SDHP_OPENVSHOP *)aRecv);
		}break;
		case 0xFA:
		{
			switch(aRecv[3])
			{
				case 0x00:
				{
					gui.SEND_VShopInfo((SDHP_SENDSVSHOP *)aRecv);
					//VShop.SetInfo((SDHP_SENDSVSHOP *)aRecv);
				}break;
				case 0x01:
				{
					gui.SEND_VShopShow((SDHP_OPENVSHOP *)aRecv);
					//VShop.Show((SDHP_OPENVSHOP *)aRecv);
				}break;
				case 0x10:
				{
					gui.SEND_MarryInfo((SDHP_RECVMARRY *)aRecv);
				}break;
			}
			return;
		}break;

		case 0xFB:
		{
			if(aRecv[3] = 0x0F)
			{
				if(aRecv[4] == 1)
					gui.SEND_OpenMuTab();
				else
					gui.SEND_CloseMuTab();
				return;
			}
		}break;
	}
	MainProtocolCore(protoNum,aRecv,aLen,aIndex);
}


void DataSend(LPBYTE Buffer,int aLen)
{
	_asm
	{
		Mov Edi,DataSend_addr
		Push aLen
		Push Buffer
		MOV ECX,DWORD PTR DS:[DataSendThisCall_addr]
		Call Edi
	}
}

void ReadConfigFile(LPSTR FileName)
{
	int LineCount=0;
	FILE *fp;
	char sLineTxt[3000] = {0};
	BOOL bRead = FALSE;
	
	if((fp=fopen(FileName, "r")) == NULL)
	{
		return;
	}

	rewind(fp);
		
	while(fgets(sLineTxt, 3000, fp) != NULL)
	{
		BuxConvert(sLineTxt,strlen(sLineTxt));		

		if(strlen(sLineTxt)>10)
		{
			sscanf(sLineTxt, "%s %s %s %s %s %d %d", config.ip,config.ver,config.serial,config.web1,config.web2, &config.loadItems,&config.camera);
		}
	}

	rewind(fp);
	fclose(fp);
}


extern "C" __declspec(dllexport) bool Init ()
{		
	gui.LoadGUI("TitanGUIEngine.dll");

#if (LAUNCHER_CHECK==1)
	char FilePath[] = ".\\SCFMain.ini";
	if(GetPrivateProfileInt("Config", "MainLoadTroughLauncher",0, FilePath) == 1)
	{
		LPWSTR *szArglist;
		int argc = 0;
		szArglist = CommandLineToArgvW(GetCommandLineW(), &argc);
		if( NULL == szArglist )
		{
			//exit(1);
		}
		else 					
		{
			char cmdLine[512]={0};
			wsprintf(cmdLine,"%ws", szArglist[1]);
			if(strcmp(cmdLine,"runrun"))
			{
				gui.LauncherExec();
				exit(0);
				//exit(1);
			}
		}	
	}
#endif

	DWORD dwOld1;
	LPVOID lpAddress1;
	lpAddress1  = (LPVOID) (IMAGE_BASE_EXE + BASE_OF_CODE_EXE); // 
	
	if(gui.SEND_Init() == false)
		exit(0);
	
	if ( VirtualProtect (lpAddress1,HEADER_text_RSize+HEADER_rdata_RSize,PAGE_EXECUTE_READWRITE,&dwOld1 )	)
	{ 

#if (WL_PROTECT==1)

	int MyCheckVar;
	VM_START_WITHLEVEL(15)
		CHECK_PROTECTION(MyCheckVar, 0x95450743)  
		if (MyCheckVar != 0x95450743)
		{			
			exit(1);
		}
	VM_END
#endif
		*(DWORD*)(ProtocolCore_addr+1) = (DWORD)&ProtocolCore - (ProtocolCore_addr+5);

		ReadConfigFile("Config.ttc");
		gui.SEND_WebInfo(config.web1,config.web2);

		SetIP();
		
		Monitor_Start();

		if(config.loadItems == 1)
			_beginthread( ItemInit, 0, NULL  );

		if(config.camera == 1)
			Main3DInit();

		memcpy(mainSerial,config.serial,16);
		memcpy(mainVersion,config.ver,5);

		//*(BYTE*)(NewOrbsFix_addr-2) = 0xEB;
		//*(BYTE*)(NewOrbsFix_addr-1) = 0x60;
		//ReplaceBytes(NewOrbsFix_addr,0x90,65);
		
		//VirtualProtect (lpAddress1,HEADER_text_RSize+HEADER_rdata_RSize,PAGE_EXECUTE_READ,&dwOld1 );
	}
	return true;
}