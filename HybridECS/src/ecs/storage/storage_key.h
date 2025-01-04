#pragma once

#include "lib/std_lib.h"


namespace hyecs
{
    struct storage_key
    {
        friend class storage_key_registry;

    public:

        static constexpr size_t max_table_index = (1 << 16) - 1;

        enum class table_offset_t : uint32_t
        {
        }; //interal manage by chunk storage table
        struct table_index_t
        {
        private:
            union
            {
                uint32_t key;

                struct
                {
                    uint32_t m_is_sparse: 1;//todo this maybe redundant
                    uint32_t m_group_index: 15;//todo group index is not used and not set
                    uint32_t m_table_index: 16;
                };
            };

        public:
            table_index_t(uint32_t group_index, uint32_t table_index, bool is_sparse) : m_is_sparse(is_sparse), m_group_index(group_index), m_table_index(table_index)
            {
            }

            table_index_t() : key(std::numeric_limits<uint32_t>::max())
            {
            }

            bool is_sparse_storage() const { return m_is_sparse; }
            uint32_t group_index() const { return m_group_index; }
            uint32_t table_index() const { return m_table_index; }
        };

    private:
        union
        {
            uint64_t key;

            struct
            {
                table_index_t table_index;
                table_offset_t table_offset;
            };
        };

        storage_key(uint64_t key) : key(key)
        {
        }

    public:
        storage_key(table_index_t table_index, table_offset_t table_offset) : table_index(table_index), table_offset(table_offset)
        {
        }

        storage_key() : key(std::numeric_limits<uint64_t>::max())
        {
        }

        static const storage_key null;

        bool is_sparse_storage() const { return !table_index.is_sparse_storage(); }
        table_index_t get_table_index() const { return table_index; }
        table_offset_t get_table_offset() const { return table_offset; }
        uint64_t hash() const { return key; }
    };
}
