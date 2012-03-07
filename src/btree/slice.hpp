#ifndef __BTREE_SLICE_HPP__
#define __BTREE_SLICE_HPP__

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "memcached/store.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/fifo_checker.hpp"

const unsigned int STARTING_ROOT_EVICTION_PRIORITY = 2 << 16;

class backfill_callback_t;
class key_tester_t;
class got_superblock_t;

/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t :
    public get_store_t,
    public set_store_t,
    public home_thread_mixin_t
{
public:
    // Blocks
    static void create(cache_t *cache); // calls the create function below with a key range which includes all keys
    static void create(cache_t *cache, const key_range_t &key_range);

    // Blocks
    explicit btree_slice_t(cache_t *cache);

    // Blocks
    ~btree_slice_t();

    /* get_store_t interface */

    get_result_t get(const store_key_t &key, order_token_t token);
    get_result_t get(const store_key_t &key, transaction_t *txn, got_superblock_t& superblock);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key,
        boost::scoped_ptr<transaction_t>& txn, got_superblock_t& superblock);

    /* set_store_t interface */

    mutation_result_t change(const mutation_t &m, castime_t castime, order_token_t token);
    mutation_result_t change(const mutation_t &m, castime_t castime, transaction_t *txn, got_superblock_t& superblock);

    /* btree_slice_t interface */

    void backfill_delete_range(key_tester_t *tester,
                               bool left_key_supplied, const store_key_t& left_key_exclusive,
                               bool right_key_supplied, const store_key_t& right_key_inclusive,
                               order_token_t token);
    void backfill_delete_range(key_tester_t *tester,
                               bool left_key_supplied, const store_key_t& left_key_exclusive,
                               bool right_key_supplied, const store_key_t& right_key_inclusive,
                               transaction_t *txn, got_superblock_t& superblock);

    void backfill(const key_range_t& key_range, repli_timestamp_t since_when, backfill_callback_t *callback, order_token_t token);
    void backfill(const key_range_t& key_range, repli_timestamp_t since_when, backfill_callback_t *callback,
                  transaction_t *txn, got_superblock_t& superblock);

    /* These store metadata for replication. There must be a better way to store this information,
    since it really doesn't belong on the btree_slice_t! TODO: Move them elsewhere. */
    void set_replication_clock(repli_timestamp_t t, order_token_t token);
    repli_timestamp_t get_replication_clock();
    void set_last_sync(repli_timestamp_t t, order_token_t token);
    repli_timestamp_t get_last_sync();
    void set_replication_master_id(uint32_t t);
    uint32_t get_replication_master_id();
    void set_replication_slave_id(uint32_t t);
    uint32_t get_replication_slave_id();

    cache_t *cache() { return cache_; }
    boost::shared_ptr<cache_account_t> get_backfill_account() { return backfill_account; }

    plain_sink_t pre_begin_transaction_sink_;

    // read and write operations are in different buckets for when
    // they go through throttling.
    order_source_t pre_begin_transaction_read_mode_source_; // bucket 0
    order_source_t pre_begin_transaction_write_mode_source_; // bucket 1

    enum { PRE_BEGIN_TRANSACTION_READ_MODE_BUCKET = 0, PRE_BEGIN_TRANSACTION_WRITE_MODE_BUCKET = 1 };

    order_checkpoint_t post_begin_transaction_checkpoint_;
    // We put all `order_token_t`s through this.
    order_checkpoint_t order_checkpoint_;

private:
    cache_t *cache_;

    // Cache account to be used when backfilling.
    boost::shared_ptr<cache_account_t> backfill_account;

    DISABLE_COPYING(btree_slice_t);

    //Information for cache eviction
public:
    eviction_priority_t root_eviction_priority;
};

#endif /* __BTREE_SLICE_HPP__ */
