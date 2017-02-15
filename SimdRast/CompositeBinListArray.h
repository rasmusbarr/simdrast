//
//  CompositeBinListArray.h
//  SimdRast
//
//  Created by Rasmus Barringer on 2012-11-02.
//  Copyright (c) 2012 Rasmus Barringer. All rights reserved.
//

#ifndef SimdRast_CompositeBinListArray_h
#define SimdRast_CompositeBinListArray_h

#include "CompositeBinList.h"

namespace srast {

class CompositeBinListArray {
private:
	CompositeBinList* array;
	unsigned binCount;
	
public:
	CompositeBinListArray(ThreadPool& threadPool);
	
	CompositeBinList& operator [] (unsigned index) const;
	
	~CompositeBinListArray();
};

}

#endif
