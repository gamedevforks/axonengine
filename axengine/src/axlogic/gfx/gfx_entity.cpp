/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/



#include "gfx_local.h"

AX_BEGIN_NAMESPACE

GfxEntity::GfxEntity() : RenderEntity(kEffect)
{

}

GfxEntity::GfxEntity( const std::string &filename ) : RenderEntity(kEffect)
{

}

GfxEntity::~GfxEntity()
{

}

BoundingBox GfxEntity::getLocalBoundingBox()
{
	BoundingBox result = BoundingBox::UnitBox;

	return result;
}

BoundingBox GfxEntity::getBoundingBox()
{
	return getLocalBoundingBox().getTransformed(m_affineMat);
}

Primitives GfxEntity::getHitTestPrims()
{
	return Primitives();
}

void GfxEntity::frameUpdate(RenderScene *qscene)
{
	std::list<GfxObject*>::iterator it = m_objects.begin();
	for (; it != m_objects.end(); ++it) {
		GfxObject *obj = *it;
		obj->frameUpdate(qscene);
	}
}

void GfxEntity::issueToScene(RenderScene *qscene)
{
	std::list<GfxObject*>::iterator it = m_objects.begin();
	for (; it != m_objects.end(); ++it) {
		GfxObject *obj = *it;
		obj->issueToQueue(qscene);
	}
}

void GfxEntity::addObject( GfxObject *obj )
{
	m_objects.push_back(obj);
	obj->m_entity = this;
}

void GfxEntity::removeObject( GfxObject *obj )
{
	std::list<GfxObject*>::iterator it = m_objects.begin();
	for (; it != m_objects.end(); ++it) {
		if (*it == obj) {
			m_objects.erase(it);
			return;
		}
	}
}

AX_END_NAMESPACE
