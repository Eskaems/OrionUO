﻿/***********************************************************************************
**
** Target.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef TARGET_H
#define TARGET_H
//----------------------------------------------------------------------------------
#include "Multi.h"
#include "Game objects/MultiObject.h"
#include "Wisp/WispDataStream.h"
//----------------------------------------------------------------------------------
//Класс для работы с таргетом
class CTarget
{
	//Тип объекта прицела
	SETGET(uchar, Type);
	//Тип прицела
	SETGET(uchar, CursorType);
	//Мульти на таргете
	SETGET(ushort, MultiGraphic);

private:
	//Серийник объекта, к которому привязан прицел
	uint m_CursorID;

	//Флаг состояния прицела
	bool m_Targeting;

	//Текущие и последний данные прицела
	uchar m_Data[19];
	uchar m_LastData[19];

	//Список объектов мульти на таргете
	CMulti *m_Multi;

	//Добавить мульти-объект
	void AddMultiObject(CMultiObject *obj);

public:
	CTarget();
	~CTarget() {}

	//Установить данные прицела
	void SetData(WISP_DATASTREAM::CDataReader &reader);

	//Установить данные мульти-таргета
	void SetMultiData(WISP_DATASTREAM::CDataReader &reader);

	bool IsTargeting() const { return m_Targeting; }

	//Очистить таргет
	void Reset();

	//Послать таргет на объект
	void SendTargetObject(const uint &Serial);

	//Послать таргет на тайл
	void SendTargetTile(const ushort &tileID, const short &x, const short &Y, char z);

	//Послать отмену таргета
	void SendCancelTarget();

	//Послать таргет на последнюю цель
	void SendLastTarget();

	//Послать таргет
	void SendTarget();

	//Загрузить мульти-объект
	void LoadMulti(const int &x, const int &y, const char &z);

	//Выгрузить мульти-объект
	void UnloadMulti();

	//Получить объект мульти в координатах
	CMulti *GetMultiAtXY(const short &x, const short &y);
};
//----------------------------------------------------------------------------------
extern CTarget g_Target;
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
