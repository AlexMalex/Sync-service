#if !defined(OBJECT_H)
#define OBJECT_H

class TObject {
public:
	TObject();
	virtual ~TObject();
};

typedef TObject* PObject;
typedef TObject& RObject;

#endif
