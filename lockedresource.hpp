#ifndef lockedresource_hpp
#define lockedresource_hpp

#include <boost/thread/recursive_mutex.hpp>

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
        LockedObject<typename T> lock()
        {
            return LockedObject<typename T>(&object, &resource_mutex);
        }
};

template<class T>
class LockedObject
{
    friend class LockedResource<typename T>;

    private:
        T* object;
        boost::unique_lock<boost::recursive_mutex> object_lock;
        LockedObject() { };
        LockedObject(T* object, boost::recursive_mutex* rmutex)
        {
            object_lock = boost::unique_lock<boost::recursive_mutex>(*rmutex);
            this->object = object;
        }

        LockedObject& operator=(const LockedObject &lo) { abort(); return (*this); };

    public:

        /* These movers invalidate the object you copy from. */
        LockedObject(LockedObject &lo) 
        { 
            (*this) = lo;
        };
        LockedObject& operator=(LockedObject &lo)
        {
            if (this == &lo) return (*this);

            object = lo.object;
            lo.object = (T*) 0;
            object_lock.swap(lo.object_lock);

            return (*this);
        }

        T* get() { return object; };
        const T* get() const { return object; };
        
        T& operator*() { return (*object); };
        T* operator->() { return object; };
        const T& operator*() const { return (*object); };
        const T* operator->() const { return object; };
        void release() { object = (T*) 0; object_lock.unlock(); };
};

};

#endif
