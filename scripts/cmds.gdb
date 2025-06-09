set print finish off
set logging enabled on

define myfinish
    finish
end

define hookpost-myfinish
    printf "posthook start\n"
    bt
    if this != 0
        printf "PTR = %V\n", _M_ptr
        if _M_ptr != 0
            printf "REFCOUNT = %V\n", _M_refcount._M_pi->_M_use_count
        end
    else
        printf "ZERO THIS\n"
    end
    printf "\n"
    continue
end

define setup_break
  rbreak std::__shared_ptr<$arg1, .*>::$arg0
  commands
    silent
    myfinish
    continue
  end
end

define setup_destructor_break
  rbreak std::__shared_ptr<$arg1, .*>::$arg0
  commands
    silent
    printf "destructor start\n"
    bt
    printf "\n"
    continue
  end
end

define setup_break_for_connections
    set $i = 1
    while $i < $argc
        p $i
        eval "setup_break $arg0 $arg%d", $i
        set $i = $i + 1
    end
end

define setup_break_for_connection_destructors
    set $i = 1
    while $i < $argc
        p $i
        eval "setup_destructor_break $arg0 $arg%d", $i
        set $i = $i + 1
    end
end

setup_break_for_connections __shared_ptr libp2p::connection::YamuxedConnection libp2p::connection::CapableConnection libp2p::connection::SecureConnection libp2p::connection::RawConnection libp2p::connection::LayerConnection libp2p::basic::ReadWriteCloser libp2p::basic::ReadWriter

setup_break_for_connection_destructors ~__shared_ptr libp2p::connection::YamuxedConnection libp2p::connection::CapableConnection libp2p::connection::SecureConnection libp2p::connection::RawConnection libp2p::connection::LayerConnection libp2p::basic::ReadWriteCloser libp2p::basic::ReadWriter

setup_break_for_connections operator= libp2p::connection::YamuxedConnection libp2p::connection::CapableConnection libp2p::connection::SecureConnection libp2p::connection::RawConnection libp2p::connection::LayerConnection libp2p::basic::ReadWriteCloser libp2p::basic::ReadWriter

info breakpoints
