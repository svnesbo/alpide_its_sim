//-----------------------------------------------------------------------------
// Title      : Interfaces
// Project    : ALICE ITS WP10
//-----------------------------------------------------------------------------
// File       : Interfaces.hpp
// Author     : Matthias Bonora (matthias.bonora@cern.ch)
// Company    : CERN / University of Salzburg
// Created    : 2017-03-27
// Last update: 2017-03-27
// Platform   : CERN 7 (CentOs)
// Target     : Simulation
// Standard   : SystemC 2.3
//-----------------------------------------------------------------------------
// Description: Collection of common interfaces to be reused
//-----------------------------------------------------------------------------
// Copyright (c)   2017
//-----------------------------------------------------------------------------
// Revisions  :
// Date        Version  Author        Description
// 2017-03-27  1.0      mbonora        Created
//-----------------------------------------------------------------------------

#pragma once

// Ignore warnings about use of auto_ptr in SystemC library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <systemc>
#pragma GCC diagnostic pop

// Ignore warnings about functions with unused variables in TLM library
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <tlm>
#pragma GCC diagnostic pop

#include <functional>

template <typename TRequest, typename TResponse,
          typename TInterface = tlm::tlm_transport_if<TRequest,
                                                      TResponse> >
struct transport_target_socket : virtual public TInterface,
                                 public sc_core::sc_export<TInterface> {

  transport_target_socket(const char *name = 0)
      : sc_core::sc_export<TInterface>(name) {
      this->bind(*this);
  }

  void register_transport(std::function<TResponse(const TRequest &)> func) {
    m_func = func;
  }

  virtual TResponse transport(const TRequest &request) {
    return m_func(request);
  }

private:
  std::function<TResponse(const TRequest &)> m_func;
};

template <typename TPayload,
          typename TInterface = tlm::tlm_blocking_put_if<TPayload> >
struct put_if_target_socket : public sc_core::sc_export<TInterface>, virtual public TInterface {
  put_if_target_socket(const char *name = 0)
      : sc_core::sc_export<TInterface>(name) {
      this->bind(*this);
  }

    void register_put(std::function<void(const TPayload&)> func) {
        m_func = func;
    }

  virtual void put(const TPayload &t) {
      m_func(t);
  }

private:
  std::function<void(const TPayload &)> m_func;
};

template <typename TPayload,
          typename TInterface = tlm::tlm_blocking_get_if<TPayload> >
struct get_if_target_socket : public sc_core::sc_export<TInterface>, virtual public TInterface {
    get_if_target_socket(const char *name = 0)
        : sc_core::sc_export<TInterface>(name) {
        this->bind(*this);
    }

    void register_get(std::function<TPayload(tlm::tlm_tag<TPayload>*)> func) {
        m_func = func;
    }

    virtual TPayload get(tlm::tlm_tag<TPayload> *t) {
        return m_func(t);
    }

private:
    std::function<TPayload(tlm::tlm_tag<TPayload>*)> m_func;
};
