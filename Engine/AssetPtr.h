#pragma once

template <class AssetType>
class AssetPtr
{
public:
			 AssetPtr( const AssetPtr<AssetType> &ptr );
	explicit AssetPtr( AssetType* pAsset);
			 AssetPtr( AssetSlot* pSlot);
			 AssetPtr( ): m_pAssetSlot(NULL) {};
			 ~AssetPtr( );

	AssetPtr<AssetType>& operator= (const AssetPtr<AssetType>& ptr);

	AssetType* operator->( ) const
	{
		return static_cast<AssetType*>(m_pAssetSlot->pAsset);
	}

	AssetType& operator*( ) const
	{
		return *(static_cast<AssetType*>(m_pAssetSlot->pAsset));
	}

	bool operator==( AssetPtr<AssetType> asset ) const;
	bool operator!=( AssetPtr<AssetType> asset ) const;
	bool operator< ( AssetPtr<AssetType> asset ) const;

private:
	AssetSlot* m_pAssetSlot;
};

template <class AssetType>
AssetPtr<AssetType>& AssetPtr<AssetType>::operator= (const AssetPtr<AssetType>& ptr)
{
	this->~AssetPtr( );
	m_pAssetSlot = ptr.m_pAssetSlot;
	m_pAssetSlot->_incRef();
	return *this;
}

template <class AssetType>
AssetPtr<AssetType>::AssetPtr( AssetSlot* pSlot ):m_pAssetSlot(pSlot)
{
	m_pAssetSlot->_incRef();
}

template <class AssetType>
AssetPtr<AssetType>::~AssetPtr( )
{
	// If this asserts because the asset slot was deleted, 
	// then we've called this destructor too many times!
	if (m_pAssetSlot != NULL)
		m_pAssetSlot->_decRef();
}