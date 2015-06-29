/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "CCTextFieldTTF.h"

#include "basics/CCDirector.h"
#include "CCEGLView.h"

NS_CC_BEGIN

static int _calcCharCount(const char * pszText)
{
    int n = 0;
    char ch = 0;
    while ((ch = *pszText))
    {
        CC_BREAK_IF(! ch);
        
        if (0x80 != (0xC0 & ch))
        {
            ++n;
        }
        ++pszText;
    }
    return n;
}

//////////////////////////////////////////////////////////////////////////
// constructor and destructor
//////////////////////////////////////////////////////////////////////////

CCTextFieldTTF::CCTextFieldTTF()
: m_pDelegate(0)
, m_nCharCount(0)
, m_pInputText(new std::string)
, m_pPlaceHolder(new std::string)   // prevent CCLabelTTF initWithString assertion
, m_attached(false)
{
    m_ColorSpaceHolder.r = m_ColorSpaceHolder.g = m_ColorSpaceHolder.b = 127;
}

CCTextFieldTTF::~CCTextFieldTTF()
{
    CC_SAFE_DELETE(m_pInputText);
    CC_SAFE_DELETE(m_pPlaceHolder);
	CC_SAFE_RELEASE(m_pDelegate);
}

void CCTextFieldTTF::cleanup()
{
	CCLabelTTF::cleanup();
	CCTextFieldTTF::setDelegate(NULL);
}

//////////////////////////////////////////////////////////////////////////
// static constructor
//////////////////////////////////////////////////////////////////////////

CCTextFieldTTF * CCTextFieldTTF::textFieldWithPlaceHolder(const char *placeholder, const CCSize& dimensions, CCTextAlignment alignment, const char *fontName, float fontSize)
{        
    CCTextFieldTTF *pRet = new CCTextFieldTTF();
    if(pRet && pRet->initWithPlaceHolder("", dimensions, alignment, fontName, fontSize))
    {
        pRet->autorelease();
        if (placeholder)
        {
            pRet->setPlaceHolder(placeholder);
        }
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return NULL;
}

CCTextFieldTTF * CCTextFieldTTF::textFieldWithPlaceHolder(const char *placeholder, const char *fontName, float fontSize)
{
    CCTextFieldTTF *pRet = new CCTextFieldTTF();
    if(pRet && pRet->initWithString("", fontName, fontSize))
    {
        pRet->autorelease();
        if (placeholder)
        {
            pRet->setPlaceHolder(placeholder);
        }
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// initialize
//////////////////////////////////////////////////////////////////////////

bool CCTextFieldTTF::initWithPlaceHolder(const char *placeholder, const CCSize& dimensions, CCTextAlignment alignment, const char *fontName, float fontSize)
{
    if (placeholder)
    {
        CC_SAFE_DELETE(m_pPlaceHolder);
        m_pPlaceHolder = new std::string(placeholder);
    }
    return CCLabelTTF::initWithString(m_pPlaceHolder->c_str(), fontName, fontSize, dimensions, alignment);
}
bool CCTextFieldTTF::initWithPlaceHolder(const char *placeholder, const char *fontName, float fontSize)
{
    if (placeholder)
    {
        CC_SAFE_DELETE(m_pPlaceHolder);
        m_pPlaceHolder = new std::string(placeholder);
    }
    return CCLabelTTF::initWithString(m_pPlaceHolder->c_str(), fontName, fontSize);
}

//////////////////////////////////////////////////////////////////////////
// CCIMEDelegate
//////////////////////////////////////////////////////////////////////////

bool CCTextFieldTTF::attachWithIME()
{
	if (m_attached) return false;
    bool bRet = CCIMEDelegate::attachWithIME();
    if (bRet)
    {
        // open keyboard
        CCEGLView* pGlView = CCDirector::sharedDirector()->getOpenGLView();
        if (pGlView)
        {
            pGlView->setIMEKeyboardState(true);
        }
    }
    return bRet;
}

bool CCTextFieldTTF::detachWithIME()
{
	if (!m_attached) return false;
    bool bRet = CCIMEDelegate::detachWithIME();
    if (bRet)
    {
		m_attached = false;
        // close keyboard
        CCEGLView * pGlView = CCDirector::sharedDirector()->getOpenGLView();
        if (pGlView)
        {
            pGlView->setIMEKeyboardState(false);
        }
    }
    return bRet;
}

bool CCTextFieldTTF::canAttachWithIME()
{
	m_attached = true;
	return (m_pDelegate ? m_pDelegate->onTextFieldAttachWithIME(this) : true);
}

bool CCTextFieldTTF::canDetachWithIME()
{
	m_attached = false;
	return (m_pDelegate ? m_pDelegate->onTextFieldDetachWithIME(this) : true);
}

void CCTextFieldTTF::insertText(const char * text, int len)
{
    std::string sInsert(text, len);

    // insert \n means input end
    int nPos = (int)sInsert.find('\n');
    if ((int)sInsert.npos != nPos)
    {
        len = nPos;
        sInsert.erase(nPos);
    }
    
	if (len > 0)
	{
		if (m_pDelegate && m_pDelegate->onTextFieldInsertText(this, sInsert.c_str(), len))
		{
			// delegate doesn't want to insert text
			return;
		}
		m_nCharCount += _calcCharCount(sInsert.c_str());
		std::string sText(*m_pInputText);
		sText.append(sInsert);
		setText(sText.c_str());
		if (m_pDelegate)
		{
			m_pDelegate->onTextFieldInserted(this, sInsert.c_str());
		}
	}

    if ((int)sInsert.npos == nPos) {
        return;
    }
    
    // '\n' inserted, let delegate process first
    if (m_pDelegate && m_pDelegate->onTextFieldInsertText(this, "\n", 1))
    {
        return;
    }
    // if delegate hasn't processed, detach from IME by default
    detachWithIME();
}

void CCTextFieldTTF::deleteBackward()
{
    int nStrLen = (int)m_pInputText->length();
    if (! nStrLen)
    {
        // there is no string
        return;
    }

    // get the delete byte number
    int nDeleteLen = 1;    // default, erase 1 byte

    while(0x80 == (0xC0 & m_pInputText->at(nStrLen - nDeleteLen)))
    {
        ++nDeleteLen;
    }

    if (m_pDelegate && m_pDelegate->onTextFieldDeleteBackward(this, m_pInputText->c_str() + nStrLen - nDeleteLen, nDeleteLen))
    {
        // delegate doesn't wan't to delete backwards
        return;
    }

    // if all text deleted, show placeholder string
    if (nStrLen <= nDeleteLen)
    {
        CCLabelTTF::setText(m_pPlaceHolder->c_str());
		if (m_pDelegate)
		{
			m_pDelegate->onTextFieldDeleted(this, m_pInputText->c_str() + nStrLen - nDeleteLen);
		}
		CC_SAFE_DELETE(m_pInputText);
		m_pInputText = new std::string;
		m_nCharCount = 0;
        return;
    }

    // set new input text
    std::string sText(m_pInputText->c_str(), nStrLen - nDeleteLen);
	setText(sText.c_str());
	if (m_pDelegate)
	{
		m_pDelegate->onTextFieldDeleted(this, m_pInputText->c_str() + nStrLen - nDeleteLen);
	}
}

const char * CCTextFieldTTF::getContentText()
{
    return m_pInputText->c_str();
}

void CCTextFieldTTF::draw()
{
    if (m_pDelegate && m_pDelegate->onDraw(this))
    {
        return;
    }
    if (m_pInputText->length())
    {
        CCLabelTTF::draw();
        return;
    }

    // draw placeholder
    ccColor3B color = getColor();
    setColor(m_ColorSpaceHolder);
    CCLabelTTF::draw();
    setColor(color);
}

const ccColor3B& CCTextFieldTTF::getColorSpaceHolder()
{
    return m_ColorSpaceHolder;
}

void CCTextFieldTTF::setColorSpaceHolder(const ccColor3B& color)
{
    m_ColorSpaceHolder = color;
}

//////////////////////////////////////////////////////////////////////////
// properties
//////////////////////////////////////////////////////////////////////////

// input text property
void CCTextFieldTTF::setText(const char *text)
{
    CC_SAFE_DELETE(m_pInputText);

    if (text)
    {
        m_pInputText = new std::string(text);
    }
    else
    {
        m_pInputText = new std::string;
    }

    // if there is no input text, display placeholder instead
    if (! m_pInputText->length())
    {
        CCLabelTTF::setText(m_pPlaceHolder->c_str());
    }
    else
    {
		CCLabelTTF::setText(m_pInputText->c_str());
    }
    m_nCharCount = _calcCharCount(m_pInputText->c_str());
}

const char* CCTextFieldTTF::getString()
{
    return m_pInputText->c_str();
}

// place holder text property
void CCTextFieldTTF::setPlaceHolder(const char * text)
{
    CC_SAFE_DELETE(m_pPlaceHolder);
    m_pPlaceHolder = (text) ? new std::string(text) : new std::string;
    if (! m_pInputText->length())
    {
		CCLabelTTF::setText(m_pPlaceHolder->c_str());
    }
}

const char * CCTextFieldTTF::getPlaceHolder()
{
    return m_pPlaceHolder->c_str();
}

NS_CC_END
