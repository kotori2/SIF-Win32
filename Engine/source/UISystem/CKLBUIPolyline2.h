/* 
   Copyright 2013 KLab Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#ifndef CKLBUIPolyline2_h
#define CKLBUIPolyline2_h

#include "CKLBUITask.h"

/*!
* \class CKLBUIPolyline2
* \brief Polyline Task Class
* 
* CKLBUIPolyline2 allows to create Polylines.
*/
class CKLBUIPolyline2 : public CKLBUITask
{
	friend class CKLBTaskFactory<CKLBUIPolyline2>;
private:
	CKLBUIPolyline2();
	virtual ~CKLBUIPolyline2();

	bool init(CKLBUITask* pParent, CKLBNode* pNode, u32 order);
	bool initCore(u32 order);
public:
	virtual u32 getClassID();
	static CKLBUIPolyline2* create(CKLBUITask* pParent, CKLBNode* pNode, u32 order);
	bool initUI  (CLuaState& lua);
	int commandUI(CLuaState& lua, int argc, int cmd);

	void execute(u32 deltaT);
	void dieUI  ();

	inline u32 getMaxPoint()		{ return m_maxpointcount;	}
	inline void setMaxPoint(u32 maxpointcount) {
		if (m_maxpointcount != maxpointcount) {
			m_maxpointcount = maxpointcount;
			m_pPolyline2->setMaxPointCount(maxpointcount);
		}
	}

	inline virtual u32 getOrder()	{ return m_order;			}

	inline u32	getPointCount()		{ return m_pointCount;		}
	inline void setPointCount(u32 pointcount) {
		m_pointCount = pointcount;
		m_pPolyline2->setPointCount(pointcount);
		getNode()->markUpMatrix();
	}

	inline void setPoint(u32 idx, float x, float y) {
		m_pPolyline2->setPoint(idx, x, y);
		getNode()->markUpMatrix();
	}

	inline u32	getColor()			{ return m_color;			}
	inline void setColor(u32 color) {
		m_color = color;
		m_pPolyline2->setColor(color);
		getNode()->markUpColor();
	}

private:

	CKLBPolyline	*	m_pPolyline2;

	u32					m_maxpointcount;
	u32					m_pointCount;
	u32					m_order;
	u32					m_color;

	static	PROP_V2		ms_propItems[];
};


#endif // CKLBUIPolyline2_h
