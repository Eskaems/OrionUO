﻿/***********************************************************************************
**
** Packets.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "Packets.h"
#include "../Screen stages/MainScreen.h"
#include "../Screen stages/SelectTownScreen.h"
#include "../Managers/PacketManager.h"
#include "../Managers/ProfessionManager.h"
#include "../Managers/CreateCharacterManager.h"
#include "../Managers/SpeechManager.h"
#include "../Managers/ConfigManager.h"
#include "../Managers/ConnectionManager.h"
#include "../CharacterList.h"
#include "../CityList.h"
#include "../OrionUO.h"
#include "../Gumps/Gump.h"
#include "../Gumps/GumpSecureTrading.h"
#include "../Gumps/GumpTextEntryDialog.h"
#include "../Gumps/GumpBook.h"
#include "../Gumps/GumpShop.h"
//----------------------------------------------------------------------------------
CPacket::CPacket(const int &size, const bool &autoResize)
: WISP_DATASTREAM::CDataWritter(size, autoResize)
{
}
//----------------------------------------------------------------------------------
void CPacket::Send()
{
	if (m_Data.size())
		g_Orion.Send(m_Data);
}
//----------------------------------------------------------------------------------
CPacketFirstLogin::CPacketFirstLogin()
: CPacket(62)
{
	WriteUInt8(0x80);

	if (g_TheAbyss)
		m_Data[61] = 0xFF;
	else
	{
		WriteString(g_MainScreen.m_Account->c_str(), 30, false);
		WriteString(g_MainScreen.m_Password->c_str(), 30, false);
		WriteUInt8(0xFF);
	}
}
//----------------------------------------------------------------------------------
CPacketSelectServer::CPacketSelectServer(uchar index)
: CPacket(3)
{
	WriteUInt8(0xA0);
	WriteUInt8(0x00);
	WriteUInt8(index);
}
//----------------------------------------------------------------------------------
CPacketSecondLogin::CPacketSecondLogin()
: CPacket(65)
{
	WriteUInt8(0x91);
	WriteDataLE(g_GameSeed, 4);
	WriteString(g_MainScreen.m_Account->c_str(), 30, false);

	int passLen = 30;

	if (g_TheAbyss)
	{
		WriteUInt16BE(0xFF07);
		passLen = 28;
	}

	WriteString(g_MainScreen.m_Password->c_str(), passLen, false);
}
//----------------------------------------------------------------------------------
CPacketCreateCharacter::CPacketCreateCharacter(const string &name)
: CPacket(104)
{
	int skillsCount = 3;
	uint packetID = 0x00;

	if (g_PacketManager.ClientVersion >= CV_70160)
	{
		skillsCount++;
		Resize(106, true);
		packetID = 0xF8;
	}

	WriteUInt8(packetID); //packetID
	WriteUInt32BE(0xEDEDEDED);
	WriteUInt32BE(0xFFFFFFFF);
	WriteUInt8(0x00); //pattern 3
	WriteString(name.c_str(), 30, false);
	WriteUInt16BE(0x0000); //?

	uint clientFlag = 0;
	
	IFOR(i, 0, g_CharacterList.ClientFlag)
		clientFlag |= (1 << i);

	WriteUInt32BE(clientFlag); //clientflag
	WriteUInt32BE(0x00000001); //?
	WriteUInt32BE(0x00000000); //logincount

	CProfession *profession = (CProfession*)g_ProfessionManager.Selected;
	uchar val = (uchar)profession->DescriptionIndex;
	WriteUInt8(val); //profession
	Move(15); //?

	if (g_PacketManager.ClientVersion < CV_4011D)
		val = (uchar)g_CreateCharacterManager.Female;
	else
	{
		val = (uchar)g_CreateCharacterManager.Race;

		if (g_PacketManager.ClientVersion < CV_7000)
			val--;

		val = (val * 2) + (uchar)g_CreateCharacterManager.Female;
	}

	WriteUInt8(val);
	val = profession->Str;
	WriteUInt8(val);
	val = profession->Dex;
	WriteUInt8(val);
	val = profession->Int;
	WriteUInt8(val);

	IFOR(i, 0, skillsCount)
	{
		val = profession->GetSkillIndex(i);
		if (val == 0xFF)
		{
			//error, skill is not selected
			WriteUInt16BE(0x0000);
		}
		else
		{
			WriteUInt8(val);
			WriteUInt8(profession->GetSkillValue(i));
		}
	}

	WriteUInt16BE(g_CreateCharacterManager.SkinTone);
	WriteUInt16BE(g_CreateCharacterManager.GetHair(g_CreateCharacterManager.HairStyle).GraphicID);
	WriteUInt16BE(g_CreateCharacterManager.HairColor);
	WriteUInt16BE(g_CreateCharacterManager.GetBeard(g_CreateCharacterManager.BeardStyle).GraphicID);
	WriteUInt16BE(g_CreateCharacterManager.BeardColor);

	ushort location = g_SelectTownScreen.m_City->LocationIndex;

	if (g_PacketManager.ClientVersion < CV_70130)
		location--;

	WriteUInt16BE(location); //location
	WriteUInt16BE(0x0000); //?

	ushort slot = 0xFFFF;
	IFOR(i, 0, g_CharacterList.Count)
	{
		if (!g_CharacterList.GetName(i).length())
		{
			slot = (ushort)i;
			break;
		}
	}

	WriteUInt16BE(slot);

	WriteDataLE(g_ConnectionManager.GetClientIP(), 4);
	WriteUInt16BE(g_CreateCharacterManager.ShirtColor);
	WriteUInt16BE(g_CreateCharacterManager.PantsColor);
}
//----------------------------------------------------------------------------------
CPacketDeleteCharacter::CPacketDeleteCharacter(const uint &charIndex)
: CPacket(39)
{
	WriteUInt8(0x83);
	Move(30); //character password
	WriteUInt32BE(charIndex);
	WriteDataLE(g_ConnectionManager.GetClientIP(), 4);
}
//----------------------------------------------------------------------------------
CPacketSelectCharacter::CPacketSelectCharacter(const uint &index, const string &name)
: CPacket(73)
{
	int copyLen = name.length();

	if (copyLen > 30)
		copyLen = 30;

	memcpy(&g_SelectedCharName[0], name.c_str(), copyLen);

	WriteUInt8(0x5D);
	WriteUInt32BE(0xEDEDEDED);
	WriteString(name.c_str(), 30, false);
	Move(2);

	uint clientFlag = 0;

	IFOR(i, 0, g_CharacterList.ClientFlag)
		clientFlag |= (1 << i);

	WriteUInt32BE(clientFlag);

	Move(24);
	WriteUInt32BE(index);
	WriteDataLE(g_ConnectionManager.GetClientIP(), 4);
}
//----------------------------------------------------------------------------------
CPacketPickupRequest::CPacketPickupRequest(uint serial, ushort count)
: CPacket(7)
{
	WriteUInt8(0x07);
	WriteUInt32BE(serial);
	WriteUInt16BE(count);
}
//----------------------------------------------------------------------------------
CPacketDropRequestOld::CPacketDropRequestOld(uint serial, ushort x, ushort y, char z, uint container)
: CPacket(14)
{
	WriteUInt8(0x08);
	WriteUInt32BE(serial);
	WriteUInt16BE(x);
	WriteUInt16BE(y);
	WriteUInt8(z);
	WriteUInt32BE(container);
}
//----------------------------------------------------------------------------------
CPacketDropRequestNew::CPacketDropRequestNew(uint serial, ushort x, ushort y, char z, uchar slot, uint container)
: CPacket(15)
{
	WriteUInt8(0x08);
	WriteUInt32BE(serial);
	WriteUInt16BE(x);
	WriteUInt16BE(y);
	WriteUInt8(z);
	WriteUInt8(slot);
	WriteUInt32BE(container);
}
//----------------------------------------------------------------------------------
CPacketEquipRequest::CPacketEquipRequest(uint serial, uchar layer, uint container)
: CPacket(10)
{
	WriteUInt8(0x13);
	WriteUInt32BE(serial);
	WriteUInt8(layer);
	WriteUInt32BE(container);
}
//----------------------------------------------------------------------------------
CPacketChangeWarmode::CPacketChangeWarmode(uchar state)
: CPacket(5)
{
	WriteUInt8(0x72);
	WriteUInt8(state);
	WriteUInt16BE(0x0032);
}
//----------------------------------------------------------------------------------
CPacketHelpRequest::CPacketHelpRequest()
: CPacket(258)
{
	WriteUInt8(0x9B);
}
//----------------------------------------------------------------------------------
CPacketStatusRequest::CPacketStatusRequest(uint serial)
: CPacket(10)
{
	WriteUInt8(0x34);
	WriteUInt32BE(0xEDEDEDED);
	WriteUInt8(4);
	WriteUInt32BE(serial);
}
//----------------------------------------------------------------------------------
CPacketSkillsRequest::CPacketSkillsRequest(uint serial)
: CPacket(10)
{
	WriteUInt8(0x34);
	WriteUInt32BE(0xEDEDEDED);
	WriteUInt8(5);
	WriteUInt32BE(serial);
}
//----------------------------------------------------------------------------------
CPacketSkillsStatusChangeRequest::CPacketSkillsStatusChangeRequest(uchar skill, uchar state)
: CPacket(6)
{
	WriteUInt8(0x3A);
	WriteUInt16BE(0x0006);
	WriteUInt16BE((ushort)skill);
	WriteUInt8(state);
}
//----------------------------------------------------------------------------------
CPacketClickRequest::CPacketClickRequest(uint serial)
: CPacket(5)
{
	WriteUInt8(0x09);
	WriteUInt32BE(serial);
}
//----------------------------------------------------------------------------------
CPacketDoubleClickRequest::CPacketDoubleClickRequest(uint serial)
: CPacket(5)
{
	WriteUInt8(0x06);
	WriteUInt32BE(serial);
}
//----------------------------------------------------------------------------------
CPacketAttackRequest::CPacketAttackRequest(uint serial)
: CPacket(5)
{
	WriteUInt8(0x05);
	WriteUInt32BE(serial);
}
//----------------------------------------------------------------------------------
CPacketClientVersion::CPacketClientVersion(string version)
: CPacket(4 + version.length())
{
	WriteUInt8(0xBD);
	WriteUInt16BE(4 + version.length());
	WriteString(version.c_str(), version.length(), false);
}
//----------------------------------------------------------------------------------
CPacketASCIISpeechRequest::CPacketASCIISpeechRequest(const char *text, SPEECH_TYPE type, ushort font, ushort color)
: CPacket(1)
{
	int len = strlen(text);
	int size = 8 + len + 1;
	Resize(size, true);

	WriteUInt8(0x03);
	WriteUInt16BE(size);
	WriteUInt8(type);
	WriteUInt16BE(color);
	WriteUInt16BE(font);
	WriteString(text, len, false);
}
//----------------------------------------------------------------------------------
CPacketUnicodeSpeechRequest::CPacketUnicodeSpeechRequest(const wchar_t *text, SPEECH_TYPE type, ushort font, ushort color, puchar language)
: CPacket(1)
{
	int len = lstrlenW(text);
	int size = 12 + (len * 2) + 2;

	uchar typeValue = (uchar)type;

	UINT_LIST codes;
	g_SpeechManager.GetKeywords(text, codes);

	//encoded
	bool encoded = codes.size() > 0;

	if (encoded)
	{
		typeValue |= ST_ENCODED_COMMAND;
		size += 4;
	}

	Resize(size, true);

	WriteUInt8(0xAD);
	WriteUInt16BE(size);
	WriteUInt8(typeValue);
	WriteUInt16BE(color);
	WriteUInt16BE(font);
	WriteDataLE(language, 4);

	//Sallos aka PlayUO algorithm
	if (encoded)
	{
		int length = codes.size();
		WriteUInt8(length >> 4);
		int num3 = length & 15;
		bool flag = false;
		int index = 0;

		while (index < length)
		{
			int keywordID = codes[index];

			if (flag)
			{
				WriteUInt8(keywordID >> 4);
				num3 = keywordID & 15;
			}
			else
			{
				WriteUInt8(((num3 << 4) | ((keywordID >> 8) & 15)));
				WriteUInt8(keywordID);
			}

			index++;
			flag = !flag;
		}

		if (!flag)
			WriteUInt8(num3 << 4);

		wstring ws(text);
		WriteString(EncodeUTF8(ws).data(), len * 2, false); //len * 2 ?????
		// в данном случае надо посылать как utf8, так читает сервер.
	}
	else
	{
		//в этом случае посылаем как юникод!?

		WriteWString(text, len, true, false);
		/*puchar str = (puchar)text;

		IFOR(i, 0, len)
		{
			*Ptr = *(str + 1);
			Ptr++;
			*Ptr = *str;
			Ptr++;
			str += 2;
		}*/
	}
}
//----------------------------------------------------------------------------------
CPacketCastSpell::CPacketCastSpell(int index)
: CPacket(1)
{
	if (g_PacketManager.ClientVersion >= CV_60142)
	{
		Resize(9, true);

		WriteUInt8(0xBF);
		WriteUInt16BE(0x0009);
		WriteUInt16BE(0x001C);
		WriteUInt16BE(0x0002);
		WriteUInt16BE(index);
	}
	else
	{
		char spell[10] = { 0 };
		sprintf_s(spell, "%i", index);

		int len = strlen(spell);
		int size = 5 + len;
		Resize(size, true);

		WriteUInt8(0x12);
		WriteUInt16BE(size);
		WriteUInt8(0x56);
		WriteString(spell, len, false);
	}
}
//----------------------------------------------------------------------------------
CPacketCastSpellFromBook::CPacketCastSpellFromBook(int index, uint serial)
: CPacket(1)
{
	char spell[25] = { 0 };
	sprintf_s(spell, "%i %d", index, (int)serial);

	int len = strlen(spell);
	int size = 5 + len;
	Resize(size, true);

	WriteUInt8(0x12);
	WriteUInt16BE(size);
	WriteUInt8(0x27);
	WriteString(spell, len, false);
}
//----------------------------------------------------------------------------------
CPacketUseSkill::CPacketUseSkill(int index)
: CPacket(10)
{
	char skill[10] = { 0 };
	sprintf_s(skill, "%d 0", index);

	int len = strlen(skill);
	int size = 5 + len;
	Resize(size, true);

	WriteUInt8(0x12);
	WriteUInt16BE(size);
	WriteUInt8(0x24);
	WriteString(skill, len, false);
}
//----------------------------------------------------------------------------------
CPacketOpenDoor::CPacketOpenDoor()
: CPacket(5)
{
	WriteUInt8(0x12);
	WriteUInt16BE(0x0005);
	WriteUInt8(0x58);
}
//----------------------------------------------------------------------------------
CPacketOpenSpellbook::CPacketOpenSpellbook(SPELLBOOK_TYPE type)
: CPacket(6)
{
	WriteUInt8(0x12);
	WriteUInt16BE(0x0006);
	WriteUInt8(0x43);
	WriteUInt8(type + 30);
}
//----------------------------------------------------------------------------------
CPacketEmoteAction::CPacketEmoteAction(const char *action)
: CPacket(1)
{
	int len = strlen(action);
	int size = 5 + len;
	Resize(size, true);

	WriteUInt8(0x12);
	WriteUInt16BE(size);
	WriteUInt8(0xC7);
	WriteString(action, len, false);
}
//----------------------------------------------------------------------------------
CPacketGumpResponse::CPacketGumpResponse(CGump *gump, int code)
: CPacket(1)
{
	int switchesCount = 0;
	int textLinesCount = 0;
	int textLinesLength = 0;

	QFOR(item, gump->m_Items, CBaseGUI*)
	{
		switch (item->GetType())
		{
			case GOT_CHECKBOX:
			case GOT_RADIO:
			{
				if (((CGUICheckbox*)item)->Checked)
					switchesCount++;

				break;
			}
			case GOT_TEXTENTRY:
			{
				CGUITextEntry *gte = (CGUITextEntry*)item;

				textLinesCount++;
				textLinesLength += (gte->m_Entry.Length() * 2);

				break;
			}
			default:
				break;
		}
	}

	int size = 19 + (switchesCount * 4) + 4 + ((textLinesCount * 4) + textLinesLength);
	Resize(size, true);

	g_PacketManager.LastGumpID = gump->ID;
	g_PacketManager.LastGumpX = gump->X;
	g_PacketManager.LastGumpY = gump->Y;

	WriteUInt8(0xB1);
	WriteUInt16BE(size);
	WriteUInt32BE(gump->Serial);
	WriteUInt32BE(gump->ID);
	WriteUInt32BE(code);
	WriteUInt32BE(switchesCount);

	puchar textptr = m_Ptr + (switchesCount * 4);
	pack32(textptr, textLinesCount);
	textptr += 4;

	QFOR(item, gump->m_Items, CBaseGUI*)
	{
		switch (item->Type)
		{
			case GOT_CHECKBOX:
			case GOT_RADIO:
			{
				if (((CGUICheckbox*)item)->Checked)
					WriteUInt32BE(((CGUICheckbox*)item)->Serial);

				break;
			}
			case GOT_TEXTENTRY:
			{
				CGUITextEntry *gte = (CGUITextEntry*)item;

				pack16(textptr, gte->Serial - 1);
				textptr += 2;
				int tlen = gte->m_Entry.Length();
				pack16(textptr, tlen);
				textptr += 2;
				const char *gteText = gte->m_Entry.c_str();

				IFOR(i, 0, tlen)
				{
					*textptr = 0;
					textptr++;
					*textptr = gteText[i];
					textptr++;
				}

				break;
			}
			default:
				break;
		}
	}
}
//----------------------------------------------------------------------------------
CPacketMenuResponse::CPacketMenuResponse(CGump *gump, int code)
: CPacket(13)
{
	WriteUInt8(0x7D);
	WriteUInt32BE(gump->Serial);
	WriteUInt16BE(gump->ID);

	if (code)
	{
		WriteUInt16BE(code);

		QFOR(item, gump->m_Items, CBaseGUI*)
		{
			if (item->Serial == code && item->Type == GOT_MENUOBJECT)
			{
				WriteUInt16BE(item->Graphic);
				WriteUInt16BE(item->Color);

				break;
			}
		}
	}
}
//----------------------------------------------------------------------------------
CPacketGrayMenuResponse::CPacketGrayMenuResponse(CGump *gump, int code)
: CPacket(13)
{
	WriteUInt8(0x7D);
	WriteUInt32BE(gump->Serial);
	WriteUInt16BE(gump->ID);
	WriteUInt16BE(code);
}
//----------------------------------------------------------------------------------
CPacketTradeResponse::CPacketTradeResponse(CGumpSecureTrading *gump, int code)
: CPacket(17)
{
	WriteUInt8(0x6F);
	WriteUInt16BE(17);

	if (code == 1) //Закрываем окно
	{
		WriteUInt8(0x01);
		WriteUInt32BE(gump->ID);
	}
	else if (code == 2) //Ткнули на чекбокс
	{
		WriteUInt8(0x02);
		WriteUInt32BE(gump->ID);
		WriteUInt32BE(gump->StateMy);
	}
}
//---------------------------------------------------------------------------
CPacketLogoutNotification::CPacketLogoutNotification()
: CPacket(5)
{
	WriteUInt8(0x01);
	WriteUInt32BE(0xFFFFFFFF);
}
//---------------------------------------------------------------------------
CPacketTextEntryDialogResponse::CPacketTextEntryDialogResponse(CGumpTextEntryDialog *gump, CEntryText *entry, bool code)
: CPacket(1)
{
	int len = entry->Length();
	int size = 12 + len + 1;
	Resize(size, true);

	WriteUInt8(0xAC);
	WriteUInt16BE(size);
	WriteUInt32BE(gump->Serial);
	WriteUInt8(gump->ButtonID);
	WriteUInt8(0);
	WriteUInt8(code ? 0x01 : 0x00);

	WriteUInt16BE(len + 1);

	WriteString(entry->c_str(), len);
}
//---------------------------------------------------------------------------
CPacketRenameRequest::CPacketRenameRequest(uint serial, string newName)
: CPacket(35)
{
	WriteUInt8(0x75);
	WriteUInt32BE(serial);
	WriteString(newName.c_str(), newName.length(), false);
}
//---------------------------------------------------------------------------
CPacketTipRequest::CPacketTipRequest(ushort id, uchar flag)
: CPacket(4)
{
	WriteUInt8(0xA7);
	WriteUInt16BE(id);
	WriteUInt8(flag);
}
//---------------------------------------------------------------------------
CPacketASCIIPromptResponse::CPacketASCIIPromptResponse(const char *text, int len, bool cancel)
: CPacket(1)
{
	int size = 15 + len + 1;
	Resize(size, true);

	WriteDataLE(g_LastASCIIPrompt, 11);
	pack16(&m_Data[0] + 1, size);
	WriteUInt32BE((int)((bool)!cancel));

	WriteString(text, len);
}
//---------------------------------------------------------------------------
CPacketUnicodePromptResponse::CPacketUnicodePromptResponse(const wchar_t *text, int len, const string &lang, bool cancel)
: CPacket(1)
{
	int size = 19 + (len * 2);
	Resize(size, true);

	WriteDataLE(g_LastUnicodePrompt, 11);
	pack16(&m_Data[0] + 1, size);
	WriteUInt32BE((int)((bool)!cancel));
	WriteString(lang, 4, false);

	WriteWString(text, len, false, false);
}
//---------------------------------------------------------------------------
CPacketDyeDataResponse::CPacketDyeDataResponse(uint serial, ushort graphic, ushort color)
: CPacket(9)
{
	WriteUInt8(0x95);
	WriteUInt32BE(serial);
	WriteUInt16BE(graphic);
	WriteUInt16BE(color);
}
//---------------------------------------------------------------------------
CPacketProfileRequest::CPacketProfileRequest(uint serial)
: CPacket(8)
{
	WriteUInt8(0xB8);
	WriteUInt16BE(0x0008);
	WriteUInt8(0);
	WriteUInt32BE(serial);
}
//---------------------------------------------------------------------------
CPacketProfileUpdate::CPacketProfileUpdate(uint serial, const wchar_t *text, int len)
: CPacket(1)
{
	int size = 12 + (len * 2);
	Resize(size, true);

	WriteUInt8(0xB8);
	WriteUInt16BE(size);
	WriteUInt8(1);
	WriteUInt32BE(serial);
	WriteUInt16BE(0x0001);
	WriteUInt16BE(len);
	WriteWString(text, len, true, false);
}
//---------------------------------------------------------------------------
CPacketCloseStatusbarGump::CPacketCloseStatusbarGump(uint serial)
: CPacket(9)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x0009);
	WriteUInt16BE(0x000C);
	WriteUInt32BE(serial);
}
//---------------------------------------------------------------------------
CPacketPartyInviteRequest::CPacketPartyInviteRequest()
: CPacket(10)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000a);
	WriteUInt16BE(0x0006);
	WriteUInt8(0x01);
	WriteUInt32BE(0x00000000);
}
//---------------------------------------------------------------------------
CPacketPartyRemoveRequest::CPacketPartyRemoveRequest(uint serial)
: CPacket(10)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000a);
	WriteUInt16BE(0x0006);
	WriteUInt8(0x02);
	WriteUInt32BE(serial);
}
//---------------------------------------------------------------------------
CPacketPartyChangeLootTypeRequest::CPacketPartyChangeLootTypeRequest(uchar type)
: CPacket(7)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x0007);
	WriteUInt16BE(0x0006);
	WriteUInt8(0x06);
	WriteUInt8(type);
}
//---------------------------------------------------------------------------
CPacketPartyAccept::CPacketPartyAccept(uint serial)
: CPacket(10)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000a);
	WriteUInt16BE(0x0006);
	WriteUInt8(0x08);
	WriteUInt32BE(serial);
}
//---------------------------------------------------------------------------
CPacketPartyDecline::CPacketPartyDecline(uint serial)
: CPacket(10)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000a);
	WriteUInt16BE(0x0006);
	WriteUInt8(0x09);
	WriteUInt32BE(serial);
}
//---------------------------------------------------------------------------
CPacketPartyMessage::CPacketPartyMessage(const wchar_t *text, int len, uint serial)
: CPacket(1)
{
	int size = 10 + (len * 2) + 2;
	Resize(size, true);

	WriteUInt8(0xBF);
	WriteUInt16BE(size);
	WriteUInt16BE(0x0006);

	if (serial) //Private message to member
	{
		WriteUInt8(0x03);
		WriteUInt32BE(serial);
	}
	else //Message to full party
		WriteUInt8(0x04);

	WriteWString(text, len, true, false);
}
//---------------------------------------------------------------------------
CPacketGameWindowSize::CPacketGameWindowSize()
: CPacket(13)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000D);
	WriteUInt16BE(0x0005);
	WriteUInt16BE(0);
	WriteUInt16BE(g_ConfigManager.GameWindowWidth);
	WriteUInt16BE(g_ConfigManager.GameWindowHeight);
	WriteUInt16BE(0);
}
//---------------------------------------------------------------------------
CPacketOpenGuildGump::CPacketOpenGuildGump()
: CPacket(10)
{
	WriteUInt8(0xD7);
	WriteUInt16BE(0x000A);
	WriteUInt16BE(0x0002);
	WriteUInt32BE(g_PlayerSerial);
	WriteUInt16BE(0x0028);
	WriteUInt16BE(0x07);
}
//---------------------------------------------------------------------------
CPacketOpenQuestGump::CPacketOpenQuestGump()
: CPacket(10)
{
	WriteUInt8(0xD7);
	WriteUInt16BE(0x000A);
	WriteUInt16BE(0x0002);
	WriteUInt32BE(g_PlayerSerial);
	WriteUInt16BE(0x0032);
	WriteUInt16BE(0x07);
}
//---------------------------------------------------------------------------
CPacketClientViewRange::CPacketClientViewRange(uchar range)
: CPacket(2)
{
	WriteUInt8(0xC8);

	if (range < MIN_VIEW_RANGE)
		range = MIN_VIEW_RANGE;
	else if (range > MAX_VIEW_RANGE)
		range = MAX_VIEW_RANGE;

	WriteUInt8(range);
}
//---------------------------------------------------------------------------
CPacketBulletinBoardRequestMessage::CPacketBulletinBoardRequestMessage(uint serial, uint msgSerial)
: CPacket(12)
{
	WriteUInt8(0x71);
	WriteUInt16BE(0x000C);
	WriteUInt8(0x03);
	WriteUInt32BE(serial);
	WriteUInt32BE(msgSerial);
}
//---------------------------------------------------------------------------
CPacketBulletinBoardRequestMessageSummary::CPacketBulletinBoardRequestMessageSummary(uint serial, uint msgSerial)
: CPacket(12)
{
	WriteUInt8(0x71);
	WriteUInt16BE(0x000C);
	WriteUInt8(0x04);
	WriteUInt32BE(serial);
	WriteUInt32BE(msgSerial);
}
//---------------------------------------------------------------------------
CPacketBulletinBoardPostMessage::CPacketBulletinBoardPostMessage(uint serial, uint replySerial, const char *subject, const char *message)
: CPacket(1)
{
	int subjectLen = strlen(subject);
	int size = 14 + subjectLen + 1;

	int lines = 1;

	int msgLen = strlen(message);
	int len = 0;

	IFOR(i, 0, msgLen)
	{
		if (message[i] == '\n')
		{
			len++;

			size += len + 1;
			len = 0;
			lines++;
		}
		else
			len++;
	}

	size += len + 2;

	Resize(size, true);

	WriteUInt8(0x71);
	WriteUInt16BE(size);
	WriteUInt8(0x05);
	WriteUInt32BE(serial);
	WriteUInt32BE(replySerial);

	WriteUInt8(subjectLen + 1);
	WriteString(subject, subjectLen, false);
	WriteUInt8(0);

	WriteUInt8(lines);
	len = 0;
	char *msgPtr = (char*)message;

	IFOR(i, 0, msgLen)
	{
		if (msgPtr[len] == '\n')
		{
			len++;

			WriteUInt8(len);

			if (len > 1)
				WriteString(msgPtr, len - 1, false);

			WriteUInt8(0);

			msgPtr += len;
			len = 0;
		}
		else
			len++;
	}

	WriteUInt8(len + 1);
	WriteString(msgPtr, len, false);
	WriteUInt8(0);
}
//---------------------------------------------------------------------------
CPacketBulletinBoardRemoveMessage::CPacketBulletinBoardRemoveMessage(uint serial, uint msgSerial)
: CPacket(12)
{
	WriteUInt8(0x71);
	WriteUInt16BE(0x000C);
	WriteUInt8(0x06);
	WriteUInt32BE(serial);
	WriteUInt32BE(msgSerial);
}
//---------------------------------------------------------------------------
CPacketAssistVersion::CPacketAssistVersion(uint version, string clientVersion)
: CPacket(1)
{
	int size = 7 + clientVersion.length();
	Resize(size, true);

	WriteUInt8(0xBE);
	WriteUInt16BE(size);
	WriteUInt32BE(version);
	WriteString(clientVersion.c_str(), clientVersion.length(), false);
}
//---------------------------------------------------------------------------
CPacketRazorAnswer::CPacketRazorAnswer()
: CPacket(4)
{
	WriteUInt8(0xF0);
	WriteUInt16BE(0x0004);
	WriteUInt8(0xFF);
}
//---------------------------------------------------------------------------
CPacketLanguage::CPacketLanguage(const string &lang)
: CPacket(1)
{
	int size = 5 + lang.length() + 1;
	Resize(size, true);

	WriteUInt8(0xBF);
	WriteUInt16BE(0x0009);
	WriteUInt16BE(0x000B);
	WriteString(lang, lang.length(), false);
}
//---------------------------------------------------------------------------
CPacketClientType::CPacketClientType()
: CPacket(10)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000A);
	WriteUInt16BE(0x000F);
	WriteUInt8(0x0A);

	uint clientFlag = 0;

	IFOR(i, 0, g_CharacterList.ClientFlag)
		clientFlag |= (1 << i);

	WriteUInt32BE(clientFlag);
}
//---------------------------------------------------------------------------
CPacketRequestPopupMenu::CPacketRequestPopupMenu(uint serial)
: CPacket(9)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x0009);
	WriteUInt16BE(0x0013);
	WriteUInt32BE(serial);
}
//---------------------------------------------------------------------------
CPacketPopupMenuSelection::CPacketPopupMenuSelection(uint serial, ushort menuID)
: CPacket(11)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000B);
	WriteUInt16BE(0x0015);
	WriteUInt32BE(serial);
	WriteUInt16BE(menuID);
}
//---------------------------------------------------------------------------
CPacketOpenChat::CPacketOpenChat(const wchar_t *name)
: CPacket(64)
{
	WriteUInt8(0xB5);

	int len = lstrlenW(name);

	if (len > 0)
	{
		if (len > 30)
			len = 30;

		WriteWString(name, len, false, false);
	}
}
//---------------------------------------------------------------------------
CPacketMapMessage::CPacketMapMessage(uint serial, MAP_MESSAGE action, uchar pin, short x, short y)
: CPacket(11)
{
	WriteUInt8(0x56);
	WriteUInt32BE(serial);
	WriteUInt8(action);
	WriteUInt8(pin);
	WriteUInt16BE(x);
	WriteUInt16BE(y);
}
//---------------------------------------------------------------------------
CPacketGuildMenuRequest::CPacketGuildMenuRequest()
: CPacket(10)
{
	WriteUInt8(0xD7);
	WriteUInt16BE(0x000A);
	WriteUInt32BE(g_PlayerSerial);
	WriteUInt16BE(0x0028);
	WriteUInt8(0x0A);
}
//---------------------------------------------------------------------------
CPacketQuestMenuRequest::CPacketQuestMenuRequest()
: CPacket(10)
{
	WriteUInt8(0xD7);
	WriteUInt16BE(0x000A);
	WriteUInt32BE(g_PlayerSerial);
	WriteUInt16BE(0x0032);
	WriteUInt8(0x0A);
}
//---------------------------------------------------------------------------
CPacketVirtureRequest::CPacketVirtureRequest(int buttonID)
: CPacket(23)
{
	WriteUInt8(0xB1);
	WriteUInt16BE(0x0017);
	WriteUInt32BE(g_PlayerSerial);
	WriteUInt32BE(0x000001CD);
	WriteUInt32BE(buttonID);
	WriteUInt32BE(0x00000001);
	WriteUInt32BE(g_PlayerSerial);
}
//---------------------------------------------------------------------------
CPacketInvokeVirtureRequest::CPacketInvokeVirtureRequest(uchar id)
: CPacket(6)
{
	WriteUInt8(0x12);
	WriteUInt16BE(0x0006);
	WriteUInt8(0xF4);
	WriteUInt8(id);
	WriteUInt8(0x00);
}
//---------------------------------------------------------------------------
CPacketMegaClilocRequestOld::CPacketMegaClilocRequestOld(const uint &serial)
: CPacket(9)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(9); //size
	WriteUInt16BE(0x0010);
	WriteUInt32BE(serial);
}
//---------------------------------------------------------------------------
CPacketMegaClilocRequest::CPacketMegaClilocRequest(const UINT_LIST &list)
: CPacket(1)
{
	int len = (int)list.size();
	int size = 3 + (len * 4);
	Resize(size, true);

	WriteUInt8(0xD6);
	WriteUInt16BE(size);

	IFOR(i, 0, len)
		WriteUInt32BE(list[i]);
}
//---------------------------------------------------------------------------
CPacketChangeStatLockStateRequest::CPacketChangeStatLockStateRequest(uchar stat, uchar state)
: CPacket(7)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x0007);
	WriteUInt16BE(0x001A);
	WriteUInt8(stat);
	WriteUInt8(state);
}
//---------------------------------------------------------------------------
CPacketBookPageData::CPacketBookPageData(CGumpBook *gump, int page)
: CPacket(1)
{
	int lineCount = 0;

	CGUITextEntry *entry = gump->GetEntry(page);

	if (entry != NULL)
	{
		CEntryText &textEntry = entry->m_Entry;
		string data = EncodeUTF8(textEntry.Data());
		int len = data.length();
		int size = 9 + 4 + 1;

		if (len)
		{
			size += len;
			const char *str = data.c_str();

			IFOR(i, 0, len)
			{
				if (*(str + i) == '\n')
					lineCount++;
			}

			if (str[len - 1] != '\n')
				lineCount++;
		}

		Resize(size, true);

		WriteUInt8(0x66);
		WriteUInt16BE(size);
		WriteUInt32BE(gump->Serial);
		WriteUInt16BE(0x0001);

		WriteUInt16BE(page);
		WriteUInt16BE(lineCount);

		if (len)
		{
			const char *str = data.c_str();

			IFOR(i, 0, len)
			{
				char ch = *(str + i);

				if (ch == '\n')
					ch = 0;

				*m_Ptr++ = ch;
			}

			*m_Ptr = 0;
		}
	}
}
//---------------------------------------------------------------------------
CPacketBuyRequest::CPacketBuyRequest(CGumpShop *gump)
: CPacket(1)
{
	int size = 8;
	int count = 0;

	QFOR(item, gump->m_ItemList[1]->m_Items, CBaseGUI*)
	{
		if (item->Type == GOT_SHOPRESULT)
		{
			size += 7;
			count++;
		}
	}

	Resize(size, true);

	WriteUInt8(0x3B);
	WriteUInt16BE(size);
	WriteUInt32BE(gump->Serial);

	if (count)
	{
		WriteUInt8(0x02);

		QFOR(item, gump->m_ItemList[1]->m_Items, CBaseGUI*)
		{
			if (item->Type == GOT_SHOPRESULT)
			{
				WriteUInt8(0x1A);
				WriteUInt32BE(item->Serial);
				WriteUInt16BE(((CGUIShopResult*)item)->m_MinMaxButtons->Value);
			}
		}
	}
	else
		WriteUInt8(0x00);
}
//---------------------------------------------------------------------------
CPacketSellRequest::CPacketSellRequest(CGumpShop *gump)
: CPacket(1)
{
	int size = 9;
	int count = 0;

	QFOR(item, gump->m_ItemList[1]->m_Items, CBaseGUI*)
	{
		if (item->Type == GOT_SHOPRESULT)
		{
			size += 6;
			count++;
		}
	}

	Resize(size, true);

	WriteUInt8(0x9F);
	WriteUInt16BE(size);
	WriteUInt32BE(gump->Serial);
	WriteUInt16BE(count);

	QFOR(item, gump->m_ItemList[1]->m_Items, CBaseGUI*)
	{
		if (item->Type == GOT_SHOPRESULT)
		{
			WriteUInt32BE(item->Serial);
			WriteUInt16BE(((CGUIShopResult*)item)->m_MinMaxButtons->Value);
		}
	}
}
//---------------------------------------------------------------------------
CPacketUseCombatAbility::CPacketUseCombatAbility(const uchar &index)
: CPacket(15)
{
	WriteUInt8(0xD7);
	WriteUInt16BE(0x000F);
	WriteUInt32BE(g_PlayerSerial);
	WriteUInt16BE(0x0019);
	WriteUInt32BE(0x00000000);
	WriteUInt8(index);
	WriteUInt8(0x0A);
}
//---------------------------------------------------------------------------
CPacketTargetSelectedObject::CPacketTargetSelectedObject(const uint &useObjectSerial, const uint &targetObjectSerial)
: CPacket(13)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000D);
	WriteUInt16BE(0x002C);
	WriteUInt32BE(useObjectSerial);
	WriteUInt32BE(targetObjectSerial);
}
//---------------------------------------------------------------------------
CPacketToggleGargoyleFlying::CPacketToggleGargoyleFlying()
: CPacket(11)
{
	WriteUInt8(0xBF);
	WriteUInt16BE(0x000B);
	WriteUInt16BE(0x0032);
	WriteUInt16BE(0x0001);
	WriteUInt32BE(0x00000000);
}
//----------------------------------------------------------------------------------
