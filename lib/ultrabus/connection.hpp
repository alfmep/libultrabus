/*
 * Copyright (C) 2026 Dan Arrhenius <dan@ultramarin.se>
 *
 * This file is part of ultrabus.
 *
 * ultrabus is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ULTRABUS_CONNECTION
#define ULTRABUS_CONNECTION

#include <ultrabus/config.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/message_filter.hpp>
#include <ultrabus/retvalue.hpp>
//#include <ultrabus/utils.hpp>
#include <condition_variable>
#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <tuple>
#include <list>
#include <array>
#include <map>
#include <cerrno>
#include <dbus/dbus.h>
#include <sys/epoll.h>


namespace ultrabus {


    /**
     * A D-Bus bus connection.
     *
     * Once connected, the object owns the underlying
     * <code>DBusConnection</code> and runs internal I/O and dispatch threads.
     * Incoming-message hooks and all asynchronous reply and name-owner
     * callbacks run on the dispatch thread. Callback arguments passed by
     * reference are valid only for the callback invocation. The destructor
     * disconnects and joins the internal threads; callers must ensure it does
     * not race with use of this object or a callback that uses it.
     */
    class connection : public message_filter {
    public:
        /**
         * Callback invoked when a bus name's owner changes.
         * @param bus_name The bus name that has an owner change.
         * @param old_owner The old owner, or an empty string if none.
         * @param new_owner The new owner, or an empty string if none.
         */
        using name_owner_changed_cb_t = std::function<void (const std::string& bus_name,
                                                            const std::string& old_owner,
                                                            const std::string& new_owner)>;

        /**
         * Callback invoked when a bus name is acquired or lost.
         * @param bus_name The bus name that is acquired or lost.
         * @param acquired <code>true</code> if the bus name
         *                 was acquired, <code>false</code> if
         *                 it was lost.
         */
        using bus_name_cb_t = std::function<void (const std::string& bus_name, bool acquired)>;


        /** Construct a disconnected connection object. */
        connection ();

        connection (const connection&) = delete;
        connection (connection&& other) = delete;

        /** Disconnect and join internal threads if the connection is open. */
        ~connection ();

        connection& operator= (const connection&) = delete;
        connection& operator= (connection&& rhs) = delete;

        /**
         * Connect to a bus.
         * @param type What bus to connect to: <code>DBUS_BUS_SESSION</code>,
         *             or <code>DBUS_BUS_SYSTEM</code>.
         * @param private_connection If <code>true</code>, make a
         *                           private connection. Default is
         *                           <code>false</code>.
         * @param exit_on_disconnect If <code>true</code>, exit the
         *                           application is the connection
         *                           is disconnected. Default is
         *                           <code>true</code>.
         * @return <code>true</code> on a successful connection, otherwise
         *         <code>false</code>. A successful call starts the internal I/O
         *         and dispatch threads.
         */
        bool connect (const DBusBusType type=DBUS_BUS_SESSION,
                      const bool private_connection=false,
                      const bool exit_on_disconnect=true);

        /**
         * Check whether the underlying D-Bus connection remains open.
         * @return <code>true</code> when connected.
         */
        bool is_connected () const;

        /**
         * Disconnect from the bus and join the internal I/O and dispatch
         * threads. Pending asynchronous calls are cancelled and their callbacks
         * are not invoked.
         */
        void disconnect ();

        /**
         * Send a message on the bus and wait for a reply.
         * @param msg The DBus message to send.
         * @param timeout The maximum time in milliseconds to wait for a message reply.
         * @return The reply message, including a D-Bus error message when the
         *         call fails. This blocks the calling thread until the reply or
         *         timeout.
         */
        message send_and_wait (message& msg, int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Send a message on the bus.
         * @param msg The DBus message to send.
         * @param reply_cb A callback called on the dispatch thread when a reply
         *        is received. The message reference is valid only during the
         *        callback. A null callback discards the reply.
         * @param timeout The maximum time in milliseconds to wait for a message
         *        reply, or <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return <code>true</code> when the pending call was queued;
         *         <code>false</code> when it could not be queued. A false
         *         result means <code>reply_cb</code> will not be invoked.
         */
        bool send (message& msg,
                   std::function<void (ultrabus::message&)> reply_cb,
                   int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Send a message on the bus without waiting for a message reply.
         * @param msg The DBus message to send.
         * @return <code>true</code> if the message was queued for sending,
         *         <code>false</code> if the message couldn't be queued for sending.
         */
        bool send (message& msg) {uint32_t s; return send(msg, s);}

        /**
         * Send a message on the bus without waiting for a message reply.
         * @param msg The DBus message to send.
         * @param serial If the message is successfully sent, the message
         *        serial number is returned in this parameter.
         * @return <code>true</code> if the message was queued for sending,
         *         <code>false</code> if the message couldn't be queued for sending.
         */
        bool send (message& msg, uint32_t& serial);

        /**
         * Request a name DBus connection name.
         * Ask the message bus to give this connection a specific name.
         * If another application already owns the name this application
         * may be put in a queue to take the ownership, it may 'steal' the
         * ownership, or simply fail. The bahavior depends on the flags
         * used when asking for ownership.
         * @param bus_name The requested bus name.
         * @param flags The following flags may be OR'ed together:
         *                - \b DBUS_NAME_FLAG_ALLOW_REPLACEMENT Allow another
         *                  application to take ownership of this name it
         *                  that application uses the flag
         *                  DBUS_NAME_FLAG_REPLACE_EXISTING.
         *                - \b DBUS_NAME_FLAG_REPLACE_EXISTING Try ot take
         *                  ownership of the name if another application
         *                  already owns it.
         *                - \b DBUS_NAME_FLAG_DO_NOT_QUEUE If taking ownership
         *                  of the name fails, do not put this application
         *                  in queue for the ownership.
         * @param timeout Timeout in milliseconds.
         * @return If the error code of the returned value is other than zero (see ultrabus::retvalue::err()),
         *         the request itself failed and the return value is invalid.<br/>
         *         The return value can be one of the following values:
         *           - \b DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER This
         *             application is now the primary owner of the name.
         *           - \b DBUS_REQUEST_NAME_REPLY_IN_QUEUE The application
         *             is currently in queue for ownership of the name.
         *           - \b DBUS_REQUEST_NAME_REPLY_EXISTS The application
         *             failed to get ownership of the name and is not put
         *             in a queue.
         *           - \b DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER The application
         *             already have ownership of the name.
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-request-name
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: RequestName</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<uint32_t> request_name (const std::string& bus_name,
                                         uint32_t flags=0,
                                         int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to request a name DBus connection name.
         * Ask the message bus to give this connection a specific name.
         * If another application already owns the name this application
         * may be put in a queue to take the ownership, it may 'steal' the
         * ownership, or simply fail. The bahavior depends on the flags
         * used when asking for ownership.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param bus_name The requested bus name.
         * @param flags The following flags may be OR'ed together:
         *                - \b DBUS_NAME_FLAG_ALLOW_REPLACEMENT Allow another
         *                  application to take ownership of this name it
         *                  that application uses the flag
         *                  DBUS_NAME_FLAG_REPLACE_EXISTING.
         *                - \b DBUS_NAME_FLAG_REPLACE_EXISTING Try ot take
         *                  ownership of the name if another application
         *                  already owns it.
         *                - \b DBUS_NAME_FLAG_DO_NOT_QUEUE If taking ownership
         *                  of the name fails, do not put this application
         *                  in queue for the ownership.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback will
         *                 have one of the following values when the error code is zero:<br/>
         *                 (if the error code is other than zero, a timeout has probably occurred,
         *                 or an invalid bus name was requested)
         *                  - \b DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER This
         *                    application is now the primary owner of the name.
         *                  - \b DBUS_REQUEST_NAME_REPLY_IN_QUEUE The application
         *                    is currently in queue for ownership of the name.
         *                  - \b DBUS_REQUEST_NAME_REPLY_EXISTS The application
         *                    failed to get ownership of the name and is not put
         *                    in a queue.
         *                  - \b DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER The application
         *                    already have ownership of the name.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> when it could not be queued. A false
         *         result means <code>callback</code> will not be invoked.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-request-name
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: RequestName</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool request_name (const std::string& bus_name,
                           uint32_t flags,
                           std::function<void (retvalue<uint32_t>& retval)> callback,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Release a previously requested DBus connection name.
         * @param bus_name The bus name to release.
         * @param timeout Timeout in milliseconds.
         * @return One of the following return values:
         *           - \b DBUS_RELEASE_NAME_REPLY_RELEASED The application
         *             has released ownership of the name.
         *           - \b DBUS_RELEASE_NAME_REPLY_NON_EXISTENT The application
         *             tried to release a name that didn't exist on the bus.
         *           - \b DBUS_RELEASE_NAME_REPLY_NOT_OWNER The application
         *             tried to release a name it didn't own.
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-release-name
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: ReleaseName</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<uint32_t> release_name (const std::string& bus_name,
                                         int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to release a previously
         * requested DBus connection name.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param bus_name The bus name to release.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-release-name
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: ReleaseName</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool release_name (const std::string& bus_name,
                           std::function<void (retvalue<uint32_t>& retval)> callback,
                           int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * List the connections currently queued for owning a bus name.
         * @param bus_name A bus name.
         * @param timeout Timeout in milliseconds.
         * @return A set of unique bus names waiting to own the specified bus name.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-list-queued-owners
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: ListQueuedOwners</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<std::set<std::string>> list_queued_owners (const std::string& bus_name,
                                                            int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to list the connections currently
         * queued for owning a bus name.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param bus_name A bus name.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-list-queued-owners
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: ListQueuedOwners</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool list_queued_owners (const std::string& bus_name,
                                 std::function<void (retvalue<std::set<std::string>>& retval)> callback,
                                 int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Return a set of all bus names.
         * @param timeout Timeout in milliseconds.
         * @return A set of all bus name.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-list-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: ListNames</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<std::set<std::string>> list_names (int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to return a set of all bus names.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-list-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: ListNames</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool list_names (std::function<void (retvalue<std::set<std::string>>& retval)> callback,
                         int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Returns a set of all names that can be activated on the bus.
         * @param timeout Timeout in milliseconds.
         * @return A set of all names that can be activated on the bus.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-list-activatable-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: ListActivatableNames</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<std::set<std::string>> list_activatable_names (int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to return a set of all names that can be activated on the bus.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-list-activatable-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: ListActivatableNames</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool list_activatable_names (std::function<void (retvalue<std::set<std::string>>& retval)> callback,
                                     int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Checks if the specified bus name exists (currently has an owner).
         * @param bus_name The bus name to query.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the specified bus name exists.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-name-exists
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: NamesHasOwner</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<bool> name_has_owner (const std::string& bus_name,
                                       int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to check if the specified bus name exists (currently has an owner).
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param bus_name The bus name to query.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-name-exists
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: NamesHasOwner</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool name_has_owner (const std::string& bus_name,
                             std::function<void (retvalue<bool>& retval)> callback,
                             int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Tries to launch the executable associated with a
         * name (service activation), as an explicit request.
         * @param service The service (bus name) to start.
         * @param flags Flags (currently not used).
         * @param timeout Timeout in milliseconds.
         * @return One of the following return values:
         *           - \b DBUS_START_REPLY_SUCCESS The service was
         *             successfully started.
         *           - \b DBUS_START_REPLY_ALREADY_RUNNING A connection
         *             already owns the given name.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-start-service-by-name
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: StartServiceByName</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<uint32_t> start_service_by_name (const std::string& service,
                                                  uint32_t flags=0,
                                                  int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to try to launch the executable associated with a
         * name (service activation), as an explicit request.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param service The service (bus name) to start.
         * @param flags Flags (currently not used).
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-start-service-by-name
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: StartServiceByName</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool start_service_by_name (const std::string& service,
                                    uint32_t flags,
                                    std::function<void (retvalue<uint32_t>& retval)> callback,
                                    int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Add to or modify the environment variables of activated services.
         * @param env The environment variables and values to add/modify.
         * @param timeout Timeout in milliseconds.
         * @return true on success, false on failure.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-update-activation-environment
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: UpdateActivationEnvironment</a>
         */
        retvalue<bool> update_activation_environment (const std::map<std::string,
                                                      std::string>& env,
                                                      int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to add to or modify the environment variables of activated services.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param env The environment variables and values to add/modify.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-update-activation-environment
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: UpdateActivationEnvironment</a>
         */
        bool update_activation_environment (const std::map<std::string, std::string>& env,
                                            std::function<void (retvalue<bool>& retval)> callback,
                                            int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Returns the unique connection name of the primary owner of the name given.
         * @param bus_name The bus name to query.
         * @param timeout Timeout in milliseconds.
         * @return A unique connection name.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-name-owner
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetNameOwner</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<std::string> get_name_owner (const std::string& bus_name,
                                              int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to get the unique connection name of the primary owner of the name given.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param bus_name The bus name to query.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-name-owner
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetNameOwner</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool get_name_owner (const std::string& bus_name,
                             std::function<void (retvalue<std::string>& retval)> callback,
                             int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Returns the Unix user ID of the process connected to the server.
         * @param service The bus name to query.
         * @param timeout Timeout in milliseconds.
         * @return A Unix user id.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-connection-unix-user
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetConnectionUnixUser</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<uint32_t> get_connection_unix_user (const std::string& service,
                                                     int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to get the Unix user ID of the process connected to the server.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param service The bus name to query.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-connection-unix-user
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetConnectionUnixUser</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool get_connection_unix_user (const std::string& service,
                                       std::function<void (retvalue<uint32_t>& retval)> callback,
                                       int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Returns the Unix process ID of the process connected to the server.
         * @param service The bus name to query.
         * @param timeout Timeout in milliseconds.
         * @return A Unix process id.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-connection-unix-process-id
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetConnectionUnixProcessID</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<uint32_t> get_connection_unix_process_id (const std::string& service,
                                                           int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to get the Unix process ID of the process connected to the server.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param service The bus name to query.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-connection-unix-process-id
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetConnectionUnixProcessID</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool get_connection_unix_process_id (const std::string& service,
                                             std::function<void (retvalue<uint32_t>& retval)> callback,
                                             int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Returns as many credentials as possible for the process connected to the server.
         * @param service The bus name to query.
         * @param timeout Timeout in milliseconds.
         * @return Connection credentials.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-connection-credentials
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetConnectionCredentials</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        retvalue<dbus_dict<dbus_string>> get_connection_credentials (const std::string& service,
                                                                     int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to get as many credentials as possible for the process connected to the server.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param service The bus name to query.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-connection-credentials
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetConnectionCredentials</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         */
        bool get_connection_credentials (
                const std::string& service,
                std::function<void (retvalue<dbus_dict<dbus_string>>& retval)> callback,
                int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Get the unique ID of the bus.
         * @param timeout Timeout in milliseconds.
         * @return Unique ID identifying the bus daemon.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-id
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetId</a>
         */
        retvalue<std::string> get_id (int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to get the unique ID of the bus.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-get-id
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: GetId</a>
         */
        bool get_id (std::function<void (retvalue<std::string>& retval)> callback,
                     int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Converts the connection into a monitor connection
         * which can be used as a debugging/monitoring tool.
         * @param rules An optional list of match rules.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> on success, <code>false</code> on failure.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-become-monitor
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: BecomeMonitor</a>
         */
        retvalue<bool> become_monitor (const std::list<std::string>& rules={},
                                       int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        // /**
        //  * Convert the connection into a monitor with no additional match rules.
        //  * @return A successful value on success; otherwise a false value with
        //  *         error details.
        //  */
        // retvalue<bool> become_monitor () {
        //     std::list<std::string> rules;
        //     return become_monitor (rules);
        // }

        /**
         * Asynchronous call to convert the connection into a monitor
         * connection which can be used as a debugging/monitoring tool.
         *
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         *
         * @param rules An optional list of match rules.
         * @param callback This callback is called when a result
         *                 is received on the message bus.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failed to queue the message.
         *
         * @see retvalue
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-become-monitor
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: BecomeMonitor</a>
         */
        bool become_monitor (const std::list<std::string>& rules,
                             std::function<void (retvalue<bool>& retval)> callback,
                             int timeout=DBUS_TIMEOUT_USE_DEFAULT);
        /**
         * Asynchronously convert the connection into an unrestricted monitor.
         * @param callback Called on the dispatch thread with the operation
         *        result; the reference is valid only during the callback.
         * @param timeout Timeout in milliseconds, or
         *        <code>DBUS_TIMEOUT_USE_DEFAULT</code>.
         * @return <code>true</code> if the request was queued; otherwise
         *         <code>false</code>, in which case no callback is made.
         */
        bool become_monitor (std::function<void (retvalue<bool>& retval)> callback,
                             int timeout=DBUS_TIMEOUT_USE_DEFAULT)
        {
            std::list<std::string> rules;
            return become_monitor (rules, callback, timeout);
        }

        /**
         * Set a callback to be called when the owner of a bus name has changed.
         * @param callback The callback to be called when the owner of a bus name has changed.
         *                 If <code>nullptr</code>, remove the callback.
         * @return <code>true</code> if the corresponding match rule was
         *         installed or removed; <code>false</code> if the connection is
         *         closed or the operation fails.
         *
         * @see name_owner_changed_cb_t
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-name-owner-changed
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: NameOwnerChanged</a>
         */
        bool set_name_owner_changed_cb (name_owner_changed_cb_t callback);

        /**
         * Set a callback to be called when the application loses ownership of a bus name.
         * @param callback The callback to be called, or <code>nullptr</code> to remove the callback.
         * @return <code>true</code> if the corresponding match rule was
         *         installed or removed; <code>false</code> if the connection is
         *         closed or the operation fails.
         *
         * @see bus_name_cb_t
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-name-lost
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: NameLost</a>
         */
        bool set_name_lost_cb (bus_name_cb_t callback);

        /**
         * Set a callback to be called when the application acquires ownership of a bus name.
         * @param callback The callback to be called, or <code>nullptr</code> to remove the callback.
         * @return <code>true</code> if the corresponding match rule was
         *         installed or removed; <code>false</code> if the connection is
         *         closed or the operation fails.
         *
         * @see bus_name_cb_t
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-name-acquired
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Messages: NameAcquired</a>
         */
        bool set_name_acquired_cb (bus_name_cb_t callback);

        /**
         * Return the underlying D-Bus connection for low-level API calls.
         * @return A borrowed pointer that remains valid only until this
         *         connection is disconnected or destroyed, or
         *         <code>nullptr</code> when disconnected.
         */
        DBusConnection* handle () {return ctx;}


    protected:
        /**
         * Handle D-Bus daemon signals used by the registered name callbacks.
         * @param msg The incoming signal.
         * @return <code>true</code> when a configured callback handles the
         *         signal; otherwise <code>false</code>.
         *
         * This method runs in the dispatch thread.
         */
        virtual bool on_signal (message& msg);


    private:
        constexpr static const int max_epoll_events = 16;
        DBusConnection* ctx;
        DBusBusType conn_type;
        std::string conn_addr;
        std::string unique_bus_name;
        std::thread io_thread;
        std::thread dispatch_thread;

        int timerfd;
        int pollfd;

        std::mutex watcher_mutex;
        std::map<int, std::map<DBusWatch*, uint32_t>> fd_map; // <fd, <watch, events>>

        std::mutex timer_mutex;
        using timer_entry_t = std::tuple<DBusTimeout*, struct timespec, unsigned>;
        std::list<timer_entry_t> timers;

        std::condition_variable dispatch_cond;
        std::mutex dispatch_cond_mutex;

        std::mutex pending_mutex;
        std::map<DBusPendingCall*, std::function<void (message&)>> pending_messages;

        std::mutex cb_mutex;
        name_owner_changed_cb_t name_owner_changed_cb;
        bus_name_cb_t name_lost_cb;
        bus_name_cb_t name_acquired_cb;

        bool private_connection;
        std::atomic_bool stop_io_thread;
        std::atomic_bool stop_dispatch_thread;

        void start_io_handler ();

        void io_handler ();
        void handle_epoll_result (std::array<struct epoll_event, max_epoll_events>& epoll_result,
                                  const int num_events);
        void handle_io_watch_event (DBusWatch* dbus_watch_obj,
                                    struct epoll_event& io_events,
                                    const uint32_t event_mask);

        void msg_dispatcher ();

        bool on_signal_impl (message& msg);

        void on_timerfd_event (uint32_t events);
        void on_pending_call (DBusPendingCall* pending);

        void on_wakeup_main ();
        void on_dispatch_status (DBusDispatchStatus new_status);

        bool on_add_watch (DBusWatch* watch);
        void on_remove_watch (DBusWatch* watch);
        void on_toggled_watch (DBusWatch* watch);

        bool on_add_timeout (DBusTimeout* timeout);
        void on_remove_timeout (DBusTimeout* timeout);
        void on_toggled_timeout (DBusTimeout* timeout);

        static std::atomic_bool dbus_threads_initialized;

        static void static_pending_call_cb (DBusPendingCall* pending, void* data);

        static void static_wakeup_main_cb (void* data);
        static void static_dispatch_status_cb (DBusConnection* connection,
                                               DBusDispatchStatus new_status,
                                               void* data);

        static dbus_bool_t static_add_watch_cb (DBusWatch* watch, void* data);
        static void static_remove_watch_cb (DBusWatch* watch, void* data);
        static void static_toggled_watch_cb (DBusWatch* watch, void* data);

        static dbus_bool_t static_add_timeout_cb (DBusTimeout* timeout, void* data);
        static void static_remove_timeout_cb (DBusTimeout* timeout, void* data);
        static void static_toggled_timeout_cb (DBusTimeout* timeout, void* data);
    };


}
#endif
