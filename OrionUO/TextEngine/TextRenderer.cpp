﻿/***********************************************************************************
**
** TextRenderer.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "TextRenderer.h"
#include "../Game objects/GameWorld.h"
#include "../SelectedObject.h"
#include "../Gumps/Gump.h"
#include "../Managers/ConfigManager.h"
#include "../Managers/AnimationManager.h"
#include "../Managers/ColorManager.h"
#include "../OrionUO.h"
#include "../Screen stages/GameScreen.h"
//----------------------------------------------------------------------------------
CTextRenderer g_WorldTextRenderer;
//----------------------------------------------------------------------------------
CTextRenderer::CTextRenderer()
: CRenderTextObject(), m_TextItems(this), m_DrawPointer(NULL)
{
}
//----------------------------------------------------------------------------------
CTextRenderer::~CTextRenderer()
{
	m_TextItems = NULL;
}
//---------------------------------------------------------------------------
CRenderTextObject *CTextRenderer::AddText(CRenderTextObject *obj)
{
	if (obj != NULL)
	{
		CRenderTextObject *item = m_TextItems;

		if (item != NULL)
		{
			if (item->m_NextDraw != NULL)
			{
				CRenderTextObject *next = item->m_NextDraw;

				item->m_NextDraw = obj;
				obj->m_PrevDraw = item;
				obj->m_NextDraw = next;
				next->m_PrevDraw = obj;
			}
			else
			{
				item->m_NextDraw = obj;
				obj->m_PrevDraw = item;
				obj->m_NextDraw = NULL;
			}
		}
	}

	return obj;
}
//---------------------------------------------------------------------------
void CTextRenderer::ToTop(CRenderTextObject *obj)
{
	obj->UnlinkDraw();

	CRenderTextObject *next = m_TextItems->m_NextDraw;

	m_TextItems->m_NextDraw = obj;
	obj->m_PrevDraw = m_TextItems;
	obj->m_NextDraw = next;

	if (next != NULL)
		next->m_PrevDraw = obj;
}
//----------------------------------------------------------------------------------
bool CTextRenderer::InRect(CTextImageBounds &rect, CRenderWorldObject *rwo)
{
	bool result = false;

	for (std::deque<CTextImageBounds>::iterator it = m_TextRect.begin(); it != m_TextRect.end(); it++)
	{
		if (it->InRect(rect))
		{
			if (rwo == NULL || rwo->TextCanBeTransparent(it->m_Text))
			{
				result = true;
				break;
			}
		}
	}

	return result;
}
//----------------------------------------------------------------------------------
bool CTextRenderer::CalculatePositions(const bool &noCalculate)
{
	bool changed = false;

	if (!noCalculate)
		ClearRect();

	for (m_DrawPointer = m_TextItems; m_DrawPointer != NULL; m_DrawPointer = m_DrawPointer->m_NextDraw)
	{
		if (!m_DrawPointer->IsText())
		{
			if (m_DrawPointer->m_NextDraw == NULL)
				break;

			continue;
		}

		CTextData *text = (CTextData*)m_DrawPointer;

		if (!noCalculate && text->Timer >= g_Ticks)
		{
			if (text->Type == TT_OBJECT)
			{
				CGameObject *go = g_World->FindWorldObject(text->Serial);

				if (go != NULL && (go->NPC || !((CGameItem*)go)->MultiBody))
				{
					CGLTextTexture &tth = text->m_Texture;
					int drawX = text->X - go->GetTextOffsetX(text);
					int drawY = text->Y - go->GetTextOffsetY(text);

					CTextImageBounds ib(text->DrawX, text->DrawY, tth.Width, tth.Height, text);

					if (text->DrawX != drawX || text->DrawY != drawY)
					{
						changed = true;

						text->DrawX = drawX;
						text->DrawY = drawY;
					}

					bool transparent = InRect(ib, go);

					if (text->Transparent != transparent)
						changed = true;

					text->Transparent = transparent;

					AddRect(ib);
				}
				else
					text->Timer = 0;
			}
			else
			{
				CGLTextTexture &tth = text->m_Texture;
				int drawX = text->X - (tth.Width / 2);
				int drawY = text->Y;

				CTextData *tdBuf = text;

				while (tdBuf != NULL)
				{
					drawY -= tdBuf->m_Texture.Height;

					tdBuf = (CTextData*)tdBuf->m_Next;
				}

				if (text->DrawX != drawX || text->DrawY != drawY)
				{
					changed = true;

					text->DrawX = drawX;
					text->DrawY = drawY;
				}

				CTextImageBounds ib(drawX, drawY, tth.Width, tth.Height, text);

				bool transparent = InRect(ib, NULL);

				if (text->Transparent != transparent)
					changed = true;

				text->Transparent = transparent;

				AddRect(ib);
			}

			if (g_ConfigManager.RemoveTextWithBlending)
			{
				int delta = text->Timer - g_Ticks;

				if (delta >= 0 && delta <= 1000)
				{
					delta = delta / 10;

					if (delta > 100)
						delta = 100;

					if (delta < 1)
						delta = 0;

					delta = (255 * delta) / 100;

					changed = true;

					if (!text->Transparent || delta <= 0x7F)
						text->Alpha = (uchar)delta;

					text->Transparent = true;
				}
			}
		}

		if (m_DrawPointer->m_NextDraw == NULL)
			break;
	}

	return changed;
}
//----------------------------------------------------------------------------------
void CTextRenderer::Draw()
{
	CalculatePositions(true);

	for (CRenderTextObject *item = m_DrawPointer; item != NULL; item = item->m_PrevDraw)
	{
		if (!item->IsText())
			continue;

		CTextData *text = (CTextData*)item;

		if (text->Timer >= g_Ticks)
		{
			ushort textColor = text->Color;

			int drawMode = 0;

			if (textColor)
			{
				g_ColorManager.SendColorsToShader(textColor);

				if (text->Unicode)
					drawMode = 3;
				else if (text->Font != 5 && text->Font != 8)
					drawMode = 2;
				else
					drawMode = 1;
			}

			glUniform1iARB(g_ShaderDrawMode, drawMode);

			if (text->Transparent)
			{
				uchar alpha = text->Alpha;

				if (alpha == 0xFF)
					alpha = 0x7F;

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4ub(0xFF, 0xFF, 0xFF, alpha);

				text->m_Texture.Draw(text->DrawX, text->DrawY);

				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				glDisable(GL_BLEND);
			}
			else
				text->m_Texture.Draw(text->DrawX, text->DrawY);
		}
	}
}
//----------------------------------------------------------------------------------
void CTextRenderer::Select(CGump *gump)
{
	if (gump != NULL)
		CalculatePositions(true);
	else
		CalculateWorldPositions(true);

	for (CRenderTextObject *item = m_DrawPointer; item != NULL; item = item->m_PrevDraw)
	{
		if (!item->IsText())
			continue;

		CTextData *text = (CTextData*)item;

		if (text->Timer >= g_Ticks && text->m_Texture.UnderMouse(text->DrawX, text->DrawY))
			g_SelectedObject.Init(item, gump);
	}
}
//----------------------------------------------------------------------------------
bool CTextRenderer::CalculateWorldPositions(const bool &noCalculate)
{
	bool changed = false;

	if (!noCalculate)
		ClearRect();

	for (m_DrawPointer = m_TextItems; m_DrawPointer != NULL; m_DrawPointer = m_DrawPointer->m_NextDraw)
	{
		if (!m_DrawPointer->IsText())
		{
			if (m_DrawPointer->m_NextDraw == NULL)
				break;

			continue;
		}

		CTextData *text = (CTextData*)m_DrawPointer;

		if (!noCalculate && text->Timer >= g_Ticks)
		{
			CGLTextTexture &tth = text->m_Texture;

			CRenderWorldObject *rwo = NULL;
			int drawX = 0;
			int drawY = 0;

			switch (text->Type)
			{
				case TT_OBJECT:
				{
					CGameObject *go = g_World->FindWorldObject(text->Serial);

					if (go != NULL && (go->NPC || !((CGameItem*)go)->MultiBody))
					{
						rwo = go;

						int gox = go->X - g_RenderBounds.PlayerX;
						int goy = go->Y - g_RenderBounds.PlayerY;

						drawX = g_RenderBounds.GameWindowCenterX + ((gox - goy) * 22);
						drawY = ((g_RenderBounds.GameWindowCenterY + ((gox + goy) * 22)) - (go->Z * 4));

						if (go->NPC)
						{
							CGameCharacter *gc = go->GameCharacterPtr();

							drawX += gc->OffsetX;
							drawY += gc->OffsetY - gc->OffsetZ;

							ANIMATION_DIMENSIONS dims = g_AnimationManager.GetAnimationDimensions(go, 0);
							drawY -= (dims.Height + dims.CenterY) + 8;

							if (g_ConfigManager.DrawStatusState == DCSS_ABOVE)
								drawY -= 14;
						}
						else
							drawY -= (go->GetStaticData()->Height + 20);

						drawX -= go->GetTextOffsetX(text);
						drawY -= go->GetTextOffsetY(text);
					}

					break;
				}
				case TT_CLIENT:
				{
					rwo = (CRenderWorldObject*)text->Serial;

					if (rwo != NULL)
					{
						int gox = rwo->X - g_RenderBounds.PlayerX;
						int goy = rwo->Y - g_RenderBounds.PlayerY;

						drawX = g_RenderBounds.GameWindowCenterX + ((gox - goy) * 22);
						drawY = ((g_RenderBounds.GameWindowCenterY + ((gox + goy) * 22)) - (rwo->Z * 4));

						ushort gID = rwo->Graphic - 0x4000;

						drawX -= rwo->GetTextOffsetX(text);
						drawY -= (g_Orion.m_StaticData[gID / 32].Tiles[gID % 32].Height + 20) + rwo->GetTextOffsetY(text);
					}

					break;
				}
				default:
					break;
			}

			if (rwo != NULL)
			{
				CTextImageBounds ib(drawX, drawY, tth.Width, tth.Height, text);

				text->DrawX = drawX;
				text->DrawY = drawY;

				text->Transparent = InRect(ib, rwo);

				AddRect(ib);

				if (g_ConfigManager.RemoveTextWithBlending)
				{
					int delta = text->Timer - g_Ticks;

					if (delta >= 0 && delta <= 1000)
					{
						delta = delta / 10;

						if (delta > 100)
							delta = 100;

						if (delta < 1)
							delta = 0;

						delta = (255 * delta) / 100;

						if (!text->Transparent || delta <= 0x7F)
							text->Alpha = (uchar)delta;

						text->Transparent = true;
					}
				}
			}
		}

		if (m_DrawPointer->m_NextDraw == NULL)
			break;
	}

	return changed;
}
//----------------------------------------------------------------------------------
void CTextRenderer::WorldDraw()
{
	CalculateWorldPositions(true);

	for (CRenderTextObject *item = m_DrawPointer; item != NULL; item = item->m_PrevDraw)
	{
		if (!item->IsText())
			continue;

		CTextData *text = (CTextData*)item;

		if (text->Type != TT_SYSTEM && text->Timer >= g_Ticks)
		{
			ushort textColor = text->Color;

			if (g_SelectedObject.Object() == item)
			{
				CGameObject *textOwner = g_World->FindWorldObject(text->Serial);

				if (textOwner != NULL && (textOwner->NPC || textOwner->IsCorpse()))
					textColor = 0x0035;
			}

			int drawMode = 0;

			if (textColor)
			{
				g_ColorManager.SendColorsToShader(textColor);

				if (text->Unicode)
					drawMode = 3;
				else if (text->Font != 5 && text->Font != 8)
					drawMode = 2;
				else
					drawMode = 1;
			}

			glUniform1iARB(g_ShaderDrawMode, drawMode);

			if (text->Transparent)
			{
				uchar alpha = text->Alpha;

				if (alpha == 0xFF)
					alpha = 0x7F;

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glColor4ub(0xFF, 0xFF, 0xFF, alpha);

				text->m_Texture.Draw(text->DrawX, text->DrawY);

				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				glDisable(GL_BLEND);
			}
			else
				text->m_Texture.Draw(text->DrawX, text->DrawY);
		}
	}
}
//----------------------------------------------------------------------------------
