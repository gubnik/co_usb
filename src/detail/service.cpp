#include <co_usb/detail/service.hpp>

co_usb::detail::handler_service::handler_service (boost::capy::execution_context &ctx)
    : boost::capy::execution_context::service()

{
}

std::stop_source co_usb::detail::handler_service::stop_source ()
{
    if (!m_handler_thread)
    {
        throw std::runtime_error{"Cannot get stop source of an unitialized service!"};
    }
    return m_handler_thread->get_stop_source();
}

co_usb::detail::handler_service::~handler_service ()
{
}

void co_usb::detail::handler_service::shutdown ()
{
    if (m_handler_thread)
    {
        m_handler_thread->join();
    }
}
