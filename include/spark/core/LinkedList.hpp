#pragma once

#include "spark/core/Utility.hpp"

#include <cstddef>
#include <new>

namespace Spark {

template<typename T>
struct LinkedListNode {
    T value;
    LinkedListNode* prev = nullptr;
    LinkedListNode* next = nullptr;
};

/**
 * Doubly linked list. Stable iterators across PushBack (not after erase of node).
 */
template<typename T>
class LinkedList {
public:
    LinkedList() = default;

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    LinkedList(LinkedList&& other) noexcept
        : head(other.head), tail(other.tail), num(other.num) {
        other.head = nullptr;
        other.tail = nullptr;
        other.num = 0;
    }

    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Clear();
        head = other.head;
        tail = other.tail;
        num = other.num;
        other.head = nullptr;
        other.tail = nullptr;
        other.num = 0;
        return *this;
    }

    ~LinkedList() { Clear(); }

    [[nodiscard]] std::size_t GetSize() const noexcept { return num; }
    [[nodiscard]] bool IsEmpty() const noexcept { return num == 0; }

    void PushBack(const T& value) {
        auto* node = static_cast<LinkedListNode<T>*>(::operator new(sizeof(LinkedListNode<T>)));
        new (&node->value) T(value);
        node->prev = tail;
        node->next = nullptr;
        if (tail != nullptr) {
            tail->next = node;
        } else {
            head = node;
        }
        tail = node;
        ++num;
    }

    void PushBack(T&& value) {
        auto* node = static_cast<LinkedListNode<T>*>(::operator new(sizeof(LinkedListNode<T>)));
        new (&node->value) T(MoveTemp(value));
        node->prev = tail;
        node->next = nullptr;
        if (tail != nullptr) {
            tail->next = node;
        } else {
            head = node;
        }
        tail = node;
        ++num;
    }

    void PushFront(const T& value) {
        auto* node = static_cast<LinkedListNode<T>*>(::operator new(sizeof(LinkedListNode<T>)));
        new (&node->value) T(value);
        node->next = head;
        node->prev = nullptr;
        if (head != nullptr) {
            head->prev = node;
        } else {
            tail = node;
        }
        head = node;
        ++num;
    }

    bool PopBack(T& outValue) {
        if (tail == nullptr) {
            return false;
        }
        LinkedListNode<T>* node = tail;
        tail = node->prev;
        if (tail != nullptr) {
            tail->next = nullptr;
        } else {
            head = nullptr;
        }
        outValue = MoveTemp(node->value);
        node->value.~T();
        ::operator delete(node);
        --num;
        return true;
    }

    bool PopFront(T& outValue) {
        if (head == nullptr) {
            return false;
        }
        LinkedListNode<T>* node = head;
        head = node->next;
        if (head != nullptr) {
            head->prev = nullptr;
        } else {
            tail = nullptr;
        }
        outValue = MoveTemp(node->value);
        node->value.~T();
        ::operator delete(node);
        --num;
        return true;
    }

    void Clear() noexcept {
        LinkedListNode<T>* node = head;
        while (node != nullptr) {
            LinkedListNode<T>* next = node->next;
            node->value.~T();
            ::operator delete(node);
            node = next;
        }
        head = nullptr;
        tail = nullptr;
        num = 0;
    }

    class Iterator {
    public:
        Iterator() = default;
        explicit Iterator(LinkedListNode<T>* node) : current(node) {}

        [[nodiscard]] T& operator*() const { return current->value; }
        [[nodiscard]] T* operator->() const { return &current->value; }

        Iterator& operator++() {
            current = current->next;
            return *this;
        }

        [[nodiscard]] bool operator==(Iterator other) const { return current == other.current; }
        [[nodiscard]] bool operator!=(Iterator other) const { return current != other.current; }

    private:
        friend class LinkedList;
        LinkedListNode<T>* current = nullptr;
    };

    [[nodiscard]] Iterator Begin() { return Iterator(head); }
    [[nodiscard]] Iterator End() { return Iterator(nullptr); }

    class ConstIterator {
    public:
        ConstIterator() = default;
        explicit ConstIterator(const LinkedListNode<T>* node) : current(node) {}

        [[nodiscard]] const T& operator*() const { return current->value; }
        [[nodiscard]] const T* operator->() const { return &current->value; }

        ConstIterator& operator++() {
            current = current->next;
            return *this;
        }

        [[nodiscard]] bool operator==(ConstIterator other) const { return current == other.current; }
        [[nodiscard]] bool operator!=(ConstIterator other) const { return current != other.current; }

    private:
        const LinkedListNode<T>* current = nullptr;
    };

    [[nodiscard]] ConstIterator Begin() const { return ConstIterator(head); }
    [[nodiscard]] ConstIterator End() const { return ConstIterator(nullptr); }

private:
    LinkedListNode<T>* head = nullptr;
    LinkedListNode<T>* tail = nullptr;
    std::size_t num = 0;
};

}  // namespace Spark
