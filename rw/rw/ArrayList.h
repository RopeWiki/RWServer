//typedef CArray <int,int&> CIntArray;



template<class TYPE>
class CArrayListPtr {
	int n;
	TYPE **data;

	public:
	operator TYPE* (){
		if (!data)
			return NULL;
		return *data+n;
		}

	operator TYPE& (){
		if (!data)
			return NULL;
		return *data+n;
		}
	
	TYPE* operator->() {
		if (!data)
			return NULL;
		return *data+n;
		}

	CArrayListPtr(void)
	{
		n=0; 
		data=NULL;
	}

	CArrayListPtr(TYPE *data)
	{
		if (!data)
			{
			this->n = 0;
			this->data = NULL;
			}
		else
			{
			Log(LOGALERT, "CArrayListPtr as pointer!!!!");
			ASSERT(!"ArrayListPtr =?= data");
			}
	}

	CString Summary()
	{
		return MkString("%d@%p", n, data);
	}


	CArrayListPtr(TYPE **data, int n)
	{
		this->n = n;
		this->data = data;
	}
};


// CArrayList

template<class TYPE>
class CArrayList {
	int size;
	int maxsize;
	BOOL sorted;
	TYPE *data;
	void *sortfunc;


void Init()
{
	data = NULL;
	sortfunc = NULL;
	size = maxsize = 0;
	sorted = FALSE;

}

void Load(const CArrayList<TYPE> &other) 
{ 
	int n = other.size;
	Reset();
	SetSize(n); 
	for (int i=0; i<n; ++i)
		data[i] = other.data[i];
};

public:
	inline TYPE& operator[](int i) { ASSERT(i>=0 && i<size); return data[i]; }
	inline int GetSize(void) { return size; }
	inline int GetMemorySize(void) { return maxsize; }
	inline void Dim(int topsize) { int cursize=size; SetSize(topsize); SetSize(cursize); };
	inline TYPE& Head(void) { return *data; }
	inline TYPE& Tail(void) { return data[size-1]; }
	inline TYPE* Data(void) { return data; }
	//inline TYPE& AddTail(void) { SetSize(size+1, TRUE); return data[size-1]; }

	inline CArrayListPtr<TYPE> AddTail(void) { SetSize(size+1, TRUE); return ptr(size-1); }
	inline CArrayListPtr<TYPE> ptr(int n) { return CArrayListPtr<TYPE>(&data, n); }


CArrayList(const CArrayList<TYPE> &a)
	{
	Init();
	Load(a);
	}

CArrayList(int presetsize = 0)
	{
	Init();
	SetSize(presetsize);
	}


~CArrayList(void)
	{
	Reset();
	if (data) free(data);
	}

CArrayList<TYPE>& operator=(const CArrayList<TYPE> &other) 
{
	Load(other);
	return *this;
}


void Reset(void)
{
	SetSize(0);
	//if (data) free(data);
	//Init();
}

void Flip(void)
{
	TYPE tmp;
	int n = size/2;
	for (int i=0; i<n; ++i)
		{
		TYPE &a = data[i], &b = data[size-i-1];
		memmove(&tmp, &a, sizeof(TYPE));
		memmove(&a, &b, sizeof(TYPE));
		memmove(&b, &tmp, sizeof(TYPE));
		}
}

int InsertAt(const TYPE &n, int i)
{
	SetSize(size+1, TRUE);
	memmove(&data[i+1], &data[i], sizeof(TYPE)*(size-1-i));
	data[i] = n;
	sorted = FALSE;
	return i;
}

int AddAt(const TYPE &n, int i)
{
	if (i+1>size)
		SetSize(i+1, TRUE);
	data[i] = n;
	sorted = FALSE;
	return i;
}

inline int AddTail(const TYPE &n)
{
	return AddAt(n, size);
}

int AddTail(CArrayList<TYPE> &a)
{
	int n = 0;
	for (int i=0; i<a.GetSize(); ++i)
		n = AddTail(a[i]);
	return n;
}

int AddUnique(const TYPE &n)
{
	if (Find(n)<0)
		return AddTail(n);
	return -1;
}


int length(void) { return GetSize(); }

TYPE *push(const TYPE &n) { return &data[AddTail(n)]; }

int Delete(int pos)
{
	if (pos>=size)
		return -1;
	(data + pos)->~TYPE();
	memmove(&data[pos], &data[pos+1], (size-pos-1)*sizeof(data[0]));
	size = size-1;
	return pos;
}




void SetSize(int newsize, BOOL grow = FALSE)
{ 
	if (newsize>maxsize) // exponential expansion: 10,20,40,80,160,320,640,1280,2500+
		{
		int newsizemax, newmaxsize;
		newmaxsize = newsizemax = max(10, newsize);
		if (grow)
			newmaxsize = (max(newsizemax, maxsize + min(1000,maxsize)));
		data = (TYPE *)realloc(data, newmaxsize *sizeof(TYPE)); 
		if (!data) Log(LOGFATAL, "Out of memory!");
		// zero init
		memset(data + maxsize, 0, (newmaxsize-maxsize)*sizeof(TYPE));
		maxsize = newmaxsize;
		}
	if (newsize>size)
		{
		// call constructor
		for( int i = size; i < newsize; i++ )
#pragma push_macro("new")
#undef new
				::new( (void*)( data + i ) ) TYPE;
#pragma pop_macro("new")

		}
	if (newsize<size)
		{
		// call destructor
		for (int i=newsize; i<size; ++i)
			(data + i)->~TYPE();		
		memset(data + newsize, 0, (size-newsize)*sizeof(TYPE));
		}

	size = newsize;
	sorted = FALSE;
}


void Reset(const TYPE &n, int newsize = -1)
{
	if (newsize>0) 
		SetSize(newsize);
	for (int i=0; i<size; ++i)
		data[i] = n;
}

static int cmp(TYPE *a1, TYPE *a2)
{
	double d = *a1-*a2;
	if (d<0) return -1;
	if (d>0) return 1;
	return 0;
}

static int invcmp(TYPE *a1, TYPE *a2)
{
	double d = *a2-*a1;
	if (d<0) return 1;
	if (d>0) return -1;
	return 0;
}

int Find(void *sortfunc, const TYPE &n, int *aprox = NULL)
{
	if (sorted)
		return qfind(&n, data, size, sizeof(TYPE), (qfunc*)sortfunc, aprox);
	for (int i=0; i<size; ++i)
		if (((qfunc*)sortfunc)(&data[i],&n)==0)
			return i;
	return -1;
}

int Find(const TYPE &n, int *aprox = NULL)
{
	if (sortfunc)
		return Find(sortfunc, n, aprox);
	if (sorted>0)
		return qfind(&n, data, size, sizeof(TYPE), sorted<0 ? (qfunc*)invcmp : (qfunc*)cmp, aprox);
	for (int i=0; i<size; ++i)
		if (data[i]==n)
			return i;
	return -1;
}


void Sort(BOOL order = 1)
{
	Sort(order<0 ? invcmp : cmp);
}

void Sort(void *fcmp, BOOL perm = TRUE)
{
	if (sorted && sortfunc==fcmp)
		return;
	qsort(data, size, sizeof(TYPE), (qfunc*)(fcmp));
	if (perm)
		{
		sorted = TRUE;
		sortfunc=fcmp;
		}
}



};




typedef CArrayList <int> CIntArrayList;

typedef CArrayList <double> CDoubleArrayList;

