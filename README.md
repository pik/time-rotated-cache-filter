Time-Rotated Cache Filter
========================

Is a stream filter designed to have properties opposite to those of a Bloom-Filter - False-Positive rates are negligibly small, while False-Negatives may appear. 

The filter supports item removal and dynamic size expansion. It is designed with stream processing in mind - where the probability of items repeating is expected to drop as more of the stream is processed. 

The current version is designed to work with a fingerprint size of 64bits and a 128 bit hash (Murmur3 by default), 128 and 256 bit fingerprint versions would be easy to add.

Structure
-----------------------------------------
The time-rotated filter is a circular list of Cuckoo-Hash Caches. The Basic Cuckoo Cache is a single continuous array, insertion indexes are derived as follows: 
```
    index1 = Hash_128(x)[0]
    fingerprint = Hash_128(x)[1]
    index2 = (index1^fingerprint) % size_k
```
And has the following methods: 
```
    * cc_contains_item(cache_t * cache, uint64_t <128 bit hash> );
    * cc_remove_item(cache_t * cache, uint64_t  <128 bit hash> );
    * cc_add_item(cache_t * cache, uint64_t  <128 bit hash> );
```
When the current filter at the top of the stack is filled to MAX_CAPACITY (0.5 by default) and an item incurs a replacement and a new cache is added (if the filter has not yet reached capacity an old item is knocked out). The size of the new filter is calculated based on growth in filter size and active use - this means size should adjust dynamically up to a maximum alloted memory space.

Reducing False Negatives 
--------------------------------
If False Negative reduction is needed beyond what can reasonably be accomplished by reducing MAX_CAPACITY (at SIZE_K = 4*n, the expected false negative rate is about ~ 1/n) - there are several options.

###Stashing
A simple method to implement a Stash is simply to skirt the current filter property of only having one cache with < MAX_OCCUPANCY, objects that fail insertion in the current cache are moved into a new cache. There is a slight quirk to this method that relative-time ordering of insertions is difficult to preserve for stashed inserts. This is because there is a very small probability of the insertion process walking into a locked cycle which no longer includes the original fingerprint to be inserted - reversal of a failed insertion is expensive, popping and stashing, or discarding is generally preferable. 

###Tertiary+ (D-ary) Hashing
Another consideration is upgrading to tertiary hashing - this can maximize memory efficiency at the cost of an extra lookup and somewhat greater insertion time.

Using Time-Rotated Cache Filter
------------------------------
* See trcfconstants.h for re-compiling the filter with application specific constants.
* See test_trcf.c for example usage.

Theoretical Bounds
--------------------------------
(todo).



