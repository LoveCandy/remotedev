//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_TEST_STRING_OSTREAM_HPP
#define BEAST_TEST_STRING_OSTREAM_HPP

#include <beast/core/async_result.hpp>
#include <beast/core/bind_handler.hpp>
#include <beast/core/buffer_prefix.hpp>
#include <beast/core/error.hpp>
#include <beast/websocket/teardown.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <string>

namespace beast {
namespace test {

class string_ostream
{
    boost::asio::io_service& ios_;
    std::size_t write_max_;

public:
    std::string str;

    explicit
    string_ostream(boost::asio::io_service& ios,
        std::size_t write_max =
                (std::numeric_limits<std::size_t>::max)())
        : ios_(ios)
        , write_max_(write_max)
    {
    }

    boost::asio::io_service&
    get_io_service()
    {
        return ios_;
    }

    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers)
    {
        error_code ec;
        auto const n = read_some(buffers, ec);
        if(ec)
            throw system_error{ec};
        return n;
    }

    template<class MutableBufferSequence>
    std::size_t
    read_some(MutableBufferSequence const& buffers,
        error_code& ec)
    {
        ec = boost::asio::error::eof;
        return 0;
    }

    template<class MutableBufferSequence, class ReadHandler>
    async_return_type<
        ReadHandler, void(error_code, std::size_t)>
    async_read_some(MutableBufferSequence const& buffers,
        ReadHandler&& handler)
    {
        async_completion<ReadHandler,
            void(error_code, std::size_t)> init{handler};
        ios_.post(bind_handler(init.completion_handler,
            boost::asio::error::eof, 0));
        return init.result.get();
    }

    template<class ConstBufferSequence>
    std::size_t
    write_some(ConstBufferSequence const& buffers)
    {
        error_code ec;
        auto const n = write_some(buffers, ec);
        if(ec)
            throw system_error{ec};
        return n;
    }

    template<class ConstBufferSequence>
    std::size_t
    write_some(
        ConstBufferSequence const& buffers, error_code&)
    {
        using boost::asio::buffer_size;
        using boost::asio::buffer_cast;
        auto const n =
            (std::min)(buffer_size(buffers), write_max_);
        str.reserve(str.size() + n);
        for(auto const& buffer : buffer_prefix(n, buffers))
            str.append(buffer_cast<char const*>(buffer),
                buffer_size(buffer));
        return n;
    }

    template<class ConstBufferSequence, class WriteHandler>
    async_return_type<
        WriteHandler, void(error_code, std::size_t)>
    async_write_some(ConstBufferSequence const& buffers,
        WriteHandler&& handler)
    {
        error_code ec;
        auto const bytes_transferred = write_some(buffers, ec);
        async_completion<WriteHandler,
            void(error_code, std::size_t)> init{handler};
        get_io_service().post(
            bind_handler(init.completion_handler, ec, bytes_transferred));
        return init.result.get();
    }

    friend
    void
    teardown(websocket::teardown_tag,
        string_ostream& stream,
            boost::system::error_code& ec)
    {
    }

    template<class TeardownHandler>
    friend
    void
    async_teardown(websocket::teardown_tag,
        string_ostream& stream,
            TeardownHandler&& handler)
    {
        stream.get_io_service().post(
            bind_handler(std::move(handler),
                error_code{}));
    }
};

} // test
} // beast

#endif
