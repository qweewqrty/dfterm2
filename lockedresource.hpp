#ifndef lockedresource_hpp
#define lockedresource_hpp

#include <boost/thread/recursive_mutex.hpp>
#include "types.hpp"

namespace dfterm {

template<class T> class LockedObject;
template<class T> class LockedResource;

/* A proxy class, that can be used to force locking (mutex lock) on
   objects before use. Can't be copied. */
template<class T>
class LockedResource
{
    private:
        boost::recursive_mutex resource_mutex;
        T object;

        /* No copies */
        LockedResource& operator=(const LockedResource &lr) { return (*this); };
        LockedResource(const LockedResource &lr) { };

    public:
        ~LockedResource()
        {
            boost::lock_guard<boost::recursive_mutex> m(resource_mutex);
        };
        LockedResource()
        {
        }

        /* Locks the resource and returns pointer.
           Only one thread will be able to lock the resource
           at a time. When done with it, call LockedObject::release()
           or destroy the object. (locks are released on destructor) */
        LockedObject<T> lock()
        {
            return LockedObject<T>(&object, &resource_mutex);
        }
};

template<class T>
class LockedObject
{
    friend class LockedResource<T>;

    private:
        T* object;
        SP<boost::unique_lock<boost::recursive_mutex> > object_lock;
        LockedObject(T* object, boost::recursive_mutex* rmutex)
        {
            object_lock = SP<boost::unique_lock<boost::recursive_mutex> >(new boost::unique_lock<boost::recursive_mutex>(boost::unique_lock<boost::recursive_mutex>(*rmutex)));
            this->object = object;
        }

    public:
        LockedObject()
        {
            object = (T*) 0;
        }

        LockedObject(const LockedObject &lo) 
        {
            object = lo.object;
            object_lock = lo.object_lock;
        };
        LockedObject& operator=(const LockedObject &lo)
        {
            if (this == &lo) return (*this);

            object = lo.object;
            object_lock = lo.object_lock;

            return (*this);
        }

        T* get() { return object; };
        const T* get() const { return object; };
        
        T& operator*() { return (*object); };
        T* operator->() { return object; };
        const T& operator*() const { return (*object); };
        const T* operator->() const { return object; };
        void release() { object = (T*) 0; if (object_lock) object_lock->unlock(); };
};

};

#endif
