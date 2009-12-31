/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/

#include "../private.h"

#ifdef AX_CONFIG_OPTION_USE_SPEEDTREE_40

AX_BEGIN_NAMESPACE

	TreeActor::TreeActor(const String &filename, int seed) : RenderEntity(kSpeedTree) {
		m_treeAsset = g_treeManager->findAsset(filename, seed);
		m_treeAsset->addActor(this);

		m_instanceParam[LeafAngleScalarX] = 1;
		m_instanceParam[LeafAngleScalarY] = 1;
		setWindMatrixOffset(float(rand()) / RAND_MAX);
	}

	TreeActor::~TreeActor() {
		m_treeAsset->removeActor(this);
		SafeRelease(m_treeAsset);
	}

	BoundingBox TreeActor::getLocalBoundingBox() {
		return m_treeAsset->getBoundingBox();
	}

	BoundingBox TreeActor::getBoundingBox() {
		return getLocalBoundingBox().getTransformed(m_affineMat);
	}

	void TreeActor::frameUpdate(QueuedScene *qscene)
	{
		m_instanceParam[InstanceScale] = m_affineMat.getScales();
	}


	Primitives TreeActor::getHitTestPrims() {
		return m_treeAsset->getAllPrimitives(m_lod);
	}

	void TreeActor::issueToQueue(QueuedScene *qscene) {
		if (!r_speedtree->getBool()) {
			return;
		}

		m_treeAsset->issueToQueue(this, qscene);
	}

AX_END_NAMESPACE

#endif // AX_CONFIG_OPTION_USE_SPEEDTREE_40
