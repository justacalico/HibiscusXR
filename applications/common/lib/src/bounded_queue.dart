import 'dart:collection';

/// FIFO that drops the oldest entry once [capacity] is exceeded - the
/// log windows use this instead of an unbounded list.
class BoundedQueue<T> extends IterableBase<T> {
  BoundedQueue(this.capacity);

  final int capacity;
  final _queue = ListQueue<T>();

  void push(T element) {
    _queue.addLast(element);
    while (_queue.length > capacity) {
      _queue.removeFirst();
    }
  }

  void clear() => _queue.clear();

  @override
  Iterator<T> get iterator => _queue.iterator;

  @override
  int get length => _queue.length;

  @override
  T get first => _queue.first;

  @override
  T get last => _queue.last;
}
