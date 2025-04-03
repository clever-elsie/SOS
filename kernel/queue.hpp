#ifndef SOS_KERNEL_RING_QUEUE
#define SOS_KERNEL_RING_QUEUE
#include <cstdint>
#include <cstddef>
#include <array>
#include "error.hpp"
template<class T>
class ArrayQueue{
public:
ArrayQueue()=default;
ArrayQueue(T*buf,size_t capacity);
template<size_t N>ArrayQueue(std::array<T,N>&buf);
Error push(const T&val);
Error push(T&&val);
Error pop();
const T&front()const;
size_t size()const;
size_t capacity()const;
private:
T*d;
size_t r,w,cap,sz;
};

#include <utility>

template<class T>
ArrayQueue<T>::ArrayQueue(T*buf,size_t capacity)
:d(buf),r(0),w(0),cap(capacity),sz(0){}

template<class T>
template<size_t N>
ArrayQueue<T>::ArrayQueue(std::array<T,N>&buf):ArrayQueue(buf.data(),N){}

template<class T>
Error ArrayQueue<T>::push(const T&val){
    if(cap==sz)return MAKE_ERROR(Error::kFull);
    d[w++]=val;
    if(++sz==cap)w=0;
    return MAKE_ERROR(Error::kSuccess);
}

template<class T>
Error ArrayQueue<T>::push(T&&val){
    if(cap==sz)return MAKE_ERROR(Error::kFull);
    d[w++]=std::move(val);
    if(++sz==cap)w=0;
    return MAKE_ERROR(Error::kSuccess);
}

template<class T>
Error ArrayQueue<T>::pop(){
    if(0==sz)return MAKE_ERROR(Error::kEmpty);
    --sz;
    if(++r==cap)r=0;
    return MAKE_ERROR(Error::kSuccess);
}

template<class T>
const T& ArrayQueue<T>::front()const{ return d[r]; }

template<class T>
size_t ArrayQueue<T>::size()const{ return sz; }

template<class T>
size_t ArrayQueue<T>::capacity()const{ return cap; }

#endif
