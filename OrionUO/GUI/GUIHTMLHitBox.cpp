﻿/***********************************************************************************
**
** GUIHTMLHitBox.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "GUIHTMLHitBox.h"
#include "GUIHTMLGump.h"
//----------------------------------------------------------------------------------
CGUIHTMLHitBox::CGUIHTMLHitBox(CGUIHTMLGump *htmlGump, const uint &serial, const int &x, const int &y, const int &width, const int &height, const bool &callOnMouseUp)
: CGUIHitBox(serial, x, y, width, height, callOnMouseUp), m_HTMLGump(htmlGump)
{
}
//----------------------------------------------------------------------------------
CGUIHTMLHitBox::~CGUIHTMLHitBox()
{
}
//----------------------------------------------------------------------------------
void CGUIHTMLHitBox::Scroll(const bool &up, const uint &delay)
{
	if (m_HTMLGump != NULL)
		m_HTMLGump->Scroll(up, delay);
}
//----------------------------------------------------------------------------------
