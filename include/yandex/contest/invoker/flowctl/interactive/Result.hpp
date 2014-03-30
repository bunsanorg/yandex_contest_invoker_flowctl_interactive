#pragma once

#include <bunsan/stream_enum.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>

namespace yandex{namespace contest{namespace invoker{
    namespace flowctl{namespace interactive
{
    struct Result
    {
        BUNSAN_INCLASS_STREAM_ENUM_CLASS(Status,
        (
            OK,
            ABNORMAL_EXIT,
            REAL_TIME_LIMIT_EXCEEDED,
            OUTPUT_LIMIT_EXCEEDED,
            INVALID_COMMAND,
            INVALID_DATA,
            INSUFFICIENT_DATA,
            EXCESS_DATA
        ))

        template <typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            ar & BOOST_SERIALIZATION_NVP(interactor);
            ar & BOOST_SERIALIZATION_NVP(solution);
        }

        Status interactor = Status::OK;
        Status solution = Status::OK;
    };
}}}}}
