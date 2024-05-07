# CRUSH: Controlled, Scalable, Decentralized Placement of Replicated Data
  CRUSH (Controlled Replication Under Scalable Hashing), a pseudo-random data distribution algorithm that efficiently and robustly distributes object replicas across a heterogeneous, structured storage cluster.
- CRUSH is implemented as a pseudo-random, deterministic function that maps an input value, typically an object or object group identifier, to a list of devices on which to store object replicas
- CRUSH needs only a compact, hierarchical description of the devices comprising the storage cluster and knowledge of the replica placement policy
  * first, it is completely distributed such that any party in a large system can independently calculate the location of any object
  * second, what little metadata is required is mostly static, changing only when devices are added or removed
- Given a single integer input value x, CRUSH will output an ordered list $\overrightarrow{R}$ of n distinct storage targets

## Hierarchical Cluster Map
The cluster map is composed of devices and buckets, both of which have numerical identifiers and weight values associated with them.
Buckets can contain any number of devices or other buckets, allowing them to form interior nodes in a storage hierarchy in which devices are always at the leaves.
- Bucket weights are defined as the sum of the weights of the items they contain.
- In contrast to conventional hashing techniques, in which any change in the number of target bins (devices) results in a massive reshuffling of bin contents, CRUSH is based on four different bucket types, each with a different selection algorithm to address data movement resulting from the addition or removal of devices and overall computational complexity

## Replica Placement
  CRUSH defines placement rules for each replication strategy or distribution policy employed that allow the storage system or administrator to specify exactly how object replicas are placed.

    For example, one might have a rule selecting a pair of targets for 2-way mirroring, one for selecting three targets in two different data centers for 3-way mirroring, one for RAID-4 over six storage devices, and so on
Each rule consists of a sequence of operations applied to
the hierarchy in a simple execution environment
## Bucket Types
CRUSH defines four different kinds of buckets to represent internal (non-leaf) nodes in the cluster hierarchy.
 * uniform buckets - contain items that are all of the same weight 
   - $c(r,x) = (hash(x)+rp) \mod m$, where $p$ is a randomly (but deterministically) chosen prime number greater
than m
 * list buckets - 
 * tree buckets
 * straw buckets

Summary of mapping speed and data reorganization efficiency of different bucket types when items are added to or removed from a bucket.
| Action    | Uniform | List    | Tree     | Straw   |
| --------- | ------- | ------- | -------- | ------- |
| Speed     | O(1)    | O(n)    | O(log n) | O(n)    |
| Additions | poor    | optimal | good     | optimal |
| Removals  | poor    | poor    | good     | optimal |

## 算法
- The $take(a)$ operation selects an item (typically a bucket) within the storage hierarchy and assigns it to the vector $\overrightarrow i$, which serves as an input to subsequent operations.
- The $select(n,t)$ operation iterates over each element $i \in \overrightarrow i$, and chooses $n$ distinct items of type $t$ in the subtree rooted at that point.

## Algorithm 1 CRUSH placement for object x 

$\text{procedure TAKE(a) \qquad}\triangleleft \text{ Put item a in working vector~i}\\
\quad\overrightarrow i\text{ ← }[a]\\
\text{end procedure}\\
\text{procedure SELECT(n,t) \qquad}\triangleleft \text{ Select n items of type t}\\
\quad\overrightarrow o\text{ ← } \phi \text{ \qquad}\triangleleft \text{ Our output, initially empty}\\
\quad\text{for }i\text{ ∈ }\overrightarrow i\text{ do \qquad}\triangleleft \text{ Loop over input~i}\\
\quad\quad f\text{ ← }0\text{ \qquad}\triangleleft \text{ No failures yet}\\
\quad\quad\text{for }r\text{ ← }1,n\text{ do \qquad}\triangleleft \text{ Loop over n replicas}\\
\quad\quad\quad f_r\text{ ← 0 \qquad}\triangleleft \text{ No failures on this replica}\\
\quad\quad\quad retry\_descent \text{ ← }false\\
\quad\quad\quad\text{repeat}\\
\quad\quad\quad\quad b\text{ ← }bucket(i) \qquad\text{}\triangleleft \text{ Start descent at bucket i}\\
\quad\quad\quad\quad\text{retry\_bucket ← false}\\
\quad\quad\quad\quad\text{repeat}\\
\quad\quad\quad\quad\quad\text{if “first n” then \qquad}\triangleleft \text{ See Section 3.2.2}\\
\quad\quad\quad\quad\quad\quad\text{r′ ← r + f}\\
\quad\quad\quad\quad\quad\text{else}\\
\quad\quad\quad\quad\quad\quad\text{r′ ← r + } f_r n \triangleleft\text{ here }f_r n\text{ means }f_r \times n\\
\quad\quad\quad\quad\quad\text{end if}\\
\quad\quad\quad\quad\quad\text{o ← b.c(r′,x) \qquad}\triangleleft \text{ See Section 3.4}\\
\quad\quad\quad\quad\quad\text{if type(o) }\ne \text{ t then}\\
\quad\quad\quad\quad\quad\quad\text{b ← bucket(o) \qquad}\triangleleft \text{ Continue descent}\\
\quad\quad\quad\quad\quad\quad\text{retry\_bucket ← true}\\
\quad\quad\quad\quad\quad\text{else if o ∈ }\overrightarrow o\text{ or failed(o) or overload(o,x) then}\\
\quad\quad\quad\quad\quad\quad f_r\text{ ← }f_r\text{ +1, f ← f +1}\\
\quad\quad\quad\quad\quad\quad\text{if o ∈ } \overrightarrow o \text{ and }f_r\text{ < 3 then}\\
\quad\quad\quad\quad\quad\quad\quad\text{retry\_bucket ← true \qquad}\triangleleft \text{ Retry collisions locally (see Section 3.2.1)}\\
\quad\quad\quad\quad\quad\quad\text{else}\\
\quad\quad\quad\quad\quad\quad\quad\text{retry\_descent ← true \qquad}\triangleleft \text{ Otherwise retry descent from i}\\
\quad\quad\quad\quad\quad\quad\text{end if}\\
\quad\quad\quad\quad\quad\text{end if}\\
\quad\quad\quad\quad\text{until ¬retry\_bucket}\\
\quad\quad\quad\text{until ¬retry\_descent}\\
\quad\quad\quad\overrightarrow o\text{ ← [}\overrightarrow o\text{,o] \qquad}\triangleleft \text{ Add o to output }\overrightarrow o\\
\quad\quad\text{end for}\\
\quad\text{end for}\\
\quad\overrightarrow i\text{ ← }\overrightarrow o\text{ \qquad}\triangleleft \text{ Copy output back into~i}\\
\text{end procedure}\\
\text{procedure EMIT \qquad}\triangleleft \text{ Append working vector }\overrightarrow i\text{ to result}\\
\quad\overrightarrow R ← [\overrightarrow R,\overrightarrow i]\\
\text{end procedure}\\
$