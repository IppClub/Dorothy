/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2008-2010 Ricardo Quesada

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
#include "CCMenu.h"
#include "basics/CCDirector.h"
#include "CCApplication.h"
#include "support/CCPointExtension.h"
#include "touch_dispatcher/CCTouchDispatcher.h"
#include "touch_dispatcher/CCTouch.h"
#include "CCStdC.h"
#include "cocoa/CCInteger.h"

#include <vector>
#include <stdarg.h>

using namespace std;

NS_CC_BEGIN

static std::vector<unsigned int> ccarray_to_std_vector(CCArray* pArray)
{
    std::vector<unsigned int> ret;
    CCObject* pObj;
    CCARRAY_FOREACH(pArray, pObj)
    {
        CCInteger* pInteger = (CCInteger*)pObj;
        ret.push_back((unsigned int)pInteger->getValue());
    }
    return ret;
}

enum 
{
    kDefaultPadding =  5,
};

//
//CCMenu
//

CCMenu* CCMenu::create(bool swallow)
{
	return CCMenu::createWithArray(nullptr, swallow);
}

CCMenu * CCMenu::create(bool swallow, CCMenuItem* item, ...)
{
    va_list args;
    va_start(args,item);
	CCArray* pArray = nullptr;
	if (item)
	{
		pArray = CCArray::create(item, nullptr);
		CCMenuItem *i = va_arg(args, CCMenuItem*);
		while (i)
		{
			pArray->addObject(i);
			i = va_arg(args, CCMenuItem*);
		}
	}
    va_end(args);
	return CCMenu::createWithArray(pArray, swallow);
}

CCMenu* CCMenu::createWithArray(CCArray* pArrayOfItems, bool swallow)
{
    CCMenu *pRet = new CCMenu(swallow);
    if (pRet && pRet->initWithArray(pArrayOfItems))
    {
        pRet->autorelease();
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
    
    return pRet;
}

CCMenu* CCMenu::createWithItem(CCMenuItem* item, bool swallow)
{
	return CCMenu::createWithArray(CCArray::create(item, nullptr), swallow);
}

bool CCMenu::init()
{
    return initWithArray(NULL);
}

bool CCMenu::initWithArray(CCArray* pArrayOfItems)
{
    if (CCLayer::init())
    {
        setTouchPriority(kCCMenuHandlerPriority);
        setTouchEnabled(true);

        m_bEnabled = true;
        // menu in the center of the screen
        CCSize s = CCDirector::sharedDirector()->getWinSize();

        this->setAnchorPoint(ccp(0.5f, 0.5f));
        this->setContentSize(s);
        
        if (pArrayOfItems != NULL)
        {
            int z = 0;
            CCObject* pObj = NULL;
            CCARRAY_FOREACH(pArrayOfItems, pObj)
            {
                CCMenuItem* item = (CCMenuItem*)pObj;
                this->addChild(item, z);
                z++;
            }
        }
    
        //    [self alignItemsVertically];
        m_pSelectedItem = NULL;
        m_eState = kCCMenuStateWaiting;
        return true;
    }
    return false;
}

void CCMenu::onExit()
{
    if (m_eState == kCCMenuStateTrackingTouch)
    {
        if (m_pSelectedItem)
        {
            m_pSelectedItem->unselected();
            m_pSelectedItem = NULL;
        }
        
        m_eState = kCCMenuStateWaiting;
    }

    CCLayer::onExit();
}

void CCMenu::removeChild(CCNode* child, bool cleanup)
{
    CCMenuItem* pMenuItem = dynamic_cast<CCMenuItem*>(child);
    
	if (pMenuItem && m_pSelectedItem == pMenuItem)
    {
        m_pSelectedItem = NULL;
    }
    
    CCNode::removeChild(child, cleanup);
}

//Menu - Events
void CCMenu::registerWithTouchDispatcher()
{
	CCTouchDispatcher* pDispatcher = CCDirector::sharedDirector()->getTouchDispatcher();
	if (m_bMultiTouches)
	{
		pDispatcher->addStandardDelegate(this, m_nTouchPriority);
	}
	else
	{
		pDispatcher->addTargetedDelegate(this, m_nTouchPriority, true);
	}
}

bool CCMenu::ccTouchBegan(CCTouch* touch, CCEvent* event)
{
    CC_UNUSED_PARAM(event);
    if (m_eState != kCCMenuStateWaiting || !m_bVisible || !m_bEnabled)
    {
        return false;
    }

    for (CCNode* c = this->m_pParent; c != NULL; c = c->getParent())
    {
        if (c->isVisible() == false)
        {
            return false;
        }
    }

    m_pSelectedItem = this->itemForTouch(touch);
    if (m_pSelectedItem)
    {
        m_eState = kCCMenuStateTrackingTouch;
        m_pSelectedItem->selected();
        return true;
    }
	if (m_bSwallowTouches && CCRect(CCPoint::zero, CCMenu::getContentSize()).containsPoint(CCMenu::convertToNodeSpace(touch->getLocation())))
	{
		m_eState = kCCMenuStateTrackingTouch;
		return true;
	}
    return false;
}

void CCMenu::ccTouchEnded(CCTouch *touch, CCEvent* event)
{
    CC_UNUSED_PARAM(touch);
    CC_UNUSED_PARAM(event);
    CCAssert(m_eState == kCCMenuStateTrackingTouch, "[Menu ccTouchEnded] -- invalid state");
	if (!m_bEnabled)
	{
		if (m_pSelectedItem)
		{
			m_pSelectedItem->unselected();
		}
		m_eState = kCCMenuStateWaiting;
		return;
	}
	if (m_pSelectedItem)
    {
        m_pSelectedItem->unselected();
        m_pSelectedItem->activate();
    }
    m_eState = kCCMenuStateWaiting;
}

void CCMenu::ccTouchCancelled(CCTouch *touch, CCEvent* event)
{
    CC_UNUSED_PARAM(touch);
    CC_UNUSED_PARAM(event);
    CCAssert(m_eState == kCCMenuStateTrackingTouch, "[Menu ccTouchCancelled] -- invalid state");
    if (m_pSelectedItem)
    {
        m_pSelectedItem->unselected();
    }
    m_eState = kCCMenuStateWaiting;
}

void CCMenu::ccTouchMoved(CCTouch* touch, CCEvent* event)
{
    CC_UNUSED_PARAM(event);
    CCAssert(m_eState == kCCMenuStateTrackingTouch, "[Menu ccTouchMoved] -- invalid state");
    CCMenuItem *currentItem = this->itemForTouch(touch);
	if (!m_bEnabled)
	{
		if (currentItem && currentItem == m_pSelectedItem)
		{
			m_pSelectedItem->unselected();
			m_pSelectedItem = NULL;
		}
		return;
	}
	if (currentItem != m_pSelectedItem) 
    {
        if (m_pSelectedItem)
        {
            m_pSelectedItem->unselected();
        }
        m_pSelectedItem = currentItem;
        if (m_pSelectedItem)
        {
            m_pSelectedItem->selected();
        }
    }
}

//Menu - Alignment
void CCMenu::alignItemsVertically()
{
    this->alignItemsVerticallyWithPadding(kDefaultPadding);
}

void CCMenu::alignItemsVerticallyWithPadding(float padding)
{
    float height = -padding;
    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCNode* pChild = dynamic_cast<CCNode*>(pObject);
            if (pChild)
            {
                height += pChild->getContentSize().height * pChild->getScaleY() + padding;
            }
        }
    }

    float y = height / 2.0f;
    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCNode* pChild = dynamic_cast<CCNode*>(pObject);
            if (pChild)
            {
                pChild->setPosition(ccp(0, y - pChild->getContentSize().height * pChild->getScaleY() / 2.0f));
                y -= pChild->getContentSize().height * pChild->getScaleY() + padding;
            }
        }
    }
}

void CCMenu::alignItemsHorizontally()
{
    this->alignItemsHorizontallyWithPadding(kDefaultPadding);
}

void CCMenu::alignItemsHorizontallyWithPadding(float padding)
{
    float width = -padding;
    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCNode* pChild = dynamic_cast<CCNode*>(pObject);
            if (pChild)
            {
                width += pChild->getContentSize().width * pChild->getScaleX() + padding;
            }
        }
    }

    float x = -width / 2.0f;
    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCNode* pChild = dynamic_cast<CCNode*>(pObject);
            if (pChild)
            {
                pChild->setPosition(ccp(x + pChild->getContentSize().width * pChild->getScaleX() / 2.0f, 0));
                 x += pChild->getContentSize().width * pChild->getScaleX() + padding;
            }
        }
    }
}

void CCMenu::alignItemsInColumns(unsigned int columns, ...)
{
    va_list args;
    va_start(args, columns);

    this->alignItemsInColumns(columns, args);

    va_end(args);
}

void CCMenu::alignItemsInColumns(unsigned int columns, va_list args)
{
    CCArray* rows = CCArray::create();
    while (columns)
    {
        rows->addObject(CCInteger::create(columns));
        columns = va_arg(args, unsigned int);
    }
    alignItemsInColumnsWithArray(rows);
}

void CCMenu::alignItemsInColumnsWithArray(CCArray* rowsArray)
{
    vector<unsigned int> rows = ccarray_to_std_vector(rowsArray);

    int height = -5;
    unsigned int row = 0;
    unsigned int rowHeight = 0;
    unsigned int columnsOccupied = 0;
    unsigned int rowColumns;

    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCNode* pChild = dynamic_cast<CCNode*>(pObject);
            if (pChild)
            {
                CCAssert(row < rows.size(), "");

                rowColumns = rows[row];
                // can not have zero columns on a row
                CCAssert(rowColumns, "");

                float tmp = pChild->getContentSize().height;
                rowHeight = (unsigned int)((rowHeight >= tmp || isnan(tmp)) ? rowHeight : tmp);

                ++columnsOccupied;
                if (columnsOccupied >= rowColumns)
                {
                    height += rowHeight + 5;

                    columnsOccupied = 0;
                    rowHeight = 0;
                    ++row;
                }
            }
        }
    }    

    // check if too many rows/columns for available menu items
    CCAssert(! columnsOccupied, "");

    CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    row = 0;
    rowHeight = 0;
    rowColumns = 0;
    float w = 0.0;
    float x = 0.0;
    float y = (float)(height / 2);

    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCNode* pChild = dynamic_cast<CCNode*>(pObject);
            if (pChild)
            {
                if (rowColumns == 0)
                {
                    rowColumns = rows[row];
                    w = winSize.width / (1 + rowColumns);
                    x = w;
                }

                float tmp = pChild->getContentSize().height;
                rowHeight = (unsigned int)((rowHeight >= tmp || isnan(tmp)) ? rowHeight : tmp);

                pChild->setPosition(ccp(x - winSize.width / 2,
                                       y - pChild->getContentSize().height / 2));

                x += w;
                ++columnsOccupied;

                if (columnsOccupied >= rowColumns)
                {
                    y -= rowHeight + 5;

                    columnsOccupied = 0;
                    rowColumns = 0;
                    rowHeight = 0;
                    ++row;
                }
            }
        }
    }    
}

void CCMenu::alignItemsInRows(unsigned int rows, ...)
{
    va_list args;
    va_start(args, rows);

    this->alignItemsInRows(rows, args);

    va_end(args);
}

void CCMenu::alignItemsInRows(unsigned int rows, va_list args)
{
    CCArray* pArray = CCArray::create();
    while (rows)
    {
        pArray->addObject(CCInteger::create(rows));
        rows = va_arg(args, unsigned int);
    }
    alignItemsInRowsWithArray(pArray);
}

void CCMenu::alignItemsInRowsWithArray(CCArray* columnArray)
{
    vector<unsigned int> columns = ccarray_to_std_vector(columnArray);

    vector<unsigned int> columnWidths;
    vector<unsigned int> columnHeights;

    int width = -10;
    int columnHeight = -5;
    unsigned int column = 0;
    unsigned int columnWidth = 0;
    unsigned int rowsOccupied = 0;
    unsigned int columnRows;

    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCNode* pChild = dynamic_cast<CCNode*>(pObject);
            if (pChild)
            {
                // check if too many menu items for the amount of rows/columns
                CCAssert(column < columns.size(), "");

                columnRows = columns[column];
                // can't have zero rows on a column
                CCAssert(columnRows, "");

                // columnWidth = fmaxf(columnWidth, [item contentSize].width);
                float tmp = pChild->getContentSize().width;
                columnWidth = (unsigned int)((columnWidth >= tmp || isnan(tmp)) ? columnWidth : tmp);

                columnHeight += (int)(pChild->getContentSize().height + 5);
                ++rowsOccupied;

                if (rowsOccupied >= columnRows)
                {
                    columnWidths.push_back(columnWidth);
                    columnHeights.push_back(columnHeight);
                    width += columnWidth + 10;

                    rowsOccupied = 0;
                    columnWidth = 0;
                    columnHeight = -5;
                    ++column;
                }
            }
        }
    }

    // check if too many rows/columns for available menu items.
    CCAssert(! rowsOccupied, "");

    CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    column = 0;
    columnWidth = 0;
    columnRows = 0;
    float x = (float)(-width / 2);
    float y = 0.0;

    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCNode* pChild = dynamic_cast<CCNode*>(pObject);
            if (pChild)
            {
                if (columnRows == 0)
                {
                    columnRows = columns[column];
                    y = (float) columnHeights[column];
                }

                // columnWidth = fmaxf(columnWidth, [item contentSize].width);
                float tmp = pChild->getContentSize().width;
                columnWidth = (unsigned int)((columnWidth >= tmp || isnan(tmp)) ? columnWidth : tmp);

                pChild->setPosition(ccp(x + columnWidths[column] / 2,
                                       y - winSize.height / 2));

                y -= pChild->getContentSize().height + 10;
                ++rowsOccupied;

                if (rowsOccupied >= columnRows)
                {
                    x += columnWidth + 5;
                    rowsOccupied = 0;
                    columnRows = 0;
                    columnWidth = 0;
                    ++column;
                }
            }
        }
    }
}

CCMenuItem* CCMenu::itemForTouch(CCTouch *touch)
{
    CCPoint touchLocation = touch->getLocation();
	if (this->getContentSize() != CCSize::zero)
	{
		CCPoint loc = this->convertToNodeSpace(touchLocation);
		if (!CCRect(CCPoint::zero, this->getContentSize()).containsPoint(loc))
		{
			return NULL;
		}
	}
    if (m_pChildren && m_pChildren->count() > 0)
    {
        CCObject* pObject = NULL;
        CCARRAY_FOREACH(m_pChildren, pObject)
        {
            CCMenuItem* pChild = dynamic_cast<CCMenuItem*>(pObject);
            if (pChild && pChild->isVisible() && pChild->isEnabled())
            {
                CCPoint local = pChild->convertToNodeSpace(touchLocation);
				if (CCRect(CCPoint::zero, pChild->getContentSize()).containsPoint(local))
                {
                    return pChild;
                }
            }
        }
    }

    return NULL;
}

NS_CC_END
