/*
Copyright (c) 2009 AxonEngine Team

This file is part of the AxonEngine project, and may only be used, modified, 
and distributed under the terms of the AxonEngine project license, license.txt.  
By continuing to use, modify, or distribute this file you indicate that you have
read the license and understand and accept it fully.
*/


#ifndef AX_CLIENT_SPEEDTREE_MANAGER_H
#define AX_CLIENT_SPEEDTREE_MANAGER_H

AX_BEGIN_NAMESPACE

class AX_API TreeManager : public IEntityManager {
public:
	TreeManager();
	~TreeManager();

	TreeAssetPtr findAsset(const std::string &name, int seed);

	void addAsset(TreeAsset *wrapper);
	void removeAsset(TreeAsset *wrapper);
	void addTree(TreeEntity *tree);
	void removeTree(TreeEntity *tree);

	// implement IEntityManager
	virtual bool isSupportExt(const std::string &ext) const;
	virtual RenderEntity *create(const std::string &name, intptr_t arg = 0);
	virtual void updateForFrame(RenderScene *qscene );
	virtual void issueToScene(RenderScene *qscene);

private:
	typedef Dict<std::string, TreeAsset*>	TreeAssetDict;
	TreeAssetDict m_treeAssetDict;

	typedef DictSet<TreeEntity*>		TreeDict;
	TreeDict m_treeDict;

	TreeAsset *m_defaulted;
};

AX_END_NAMESPACE

#endif // end guardian

