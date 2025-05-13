package bgu.spl.net.srv;

import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class ConnectionsImpl<T> implements Connections<T> {
    // saves info about the active clients (login id, subscribed topics, etc.)

    private Map<Integer, ConnectionHandler<T>> activeClients = new ConcurrentHashMap<>();
    private Map<Integer, String> activeClientsNames = new ConcurrentHashMap<>();
    private Map<String, Set<Integer>> channels = new ConcurrentHashMap<>();
    private Map<String, String> logins = new ConcurrentHashMap<>();
    private Map<Integer, Map<String, Integer>> subscriptionIDs = new ConcurrentHashMap<>();
    private int messageId = 0;

    @Override
    public synchronized boolean send(int connectionId, T msg) {
        ConnectionHandler<T> ch = activeClients.get(connectionId);
        if (ch != null) {
            ch.send(msg);
            return true;
        }
        return false;
    }

    // Not used?
    @Override
    public synchronized void send(String channel, T msg) {
        Set<Integer> channelList = channels.get(channel);
        if (channelList != null) {
            for (Integer id : channelList)
                send(id, msg);
        }

    }

    @Override
    public synchronized void disconnect(int connectionId) {
        activeClients.remove(connectionId);
        for (Map.Entry<String, Set<Integer>> entry : channels.entrySet()) {
            // String key = entry.getKey();
            Set<Integer> value = entry.getValue();
            value.remove(connectionId);
        }
    }

    public synchronized Map<Integer, ConnectionHandler<T>> getActiveClients() {
        return activeClients;
    }

    public synchronized Map<String, Set<Integer>> getChannels() {
        return channels;
    }

    public synchronized void addActiveClient(int connectionId, ConnectionHandler<T> ch) {
        activeClients.put(connectionId, ch);
    }

    public synchronized void removeActiveClient(int connectionId) {
        activeClients.remove(connectionId);
        channels.forEach((k, v) -> v.remove(connectionId));
        subscriptionIDs.remove(connectionId);
        activeClientsNames.remove(connectionId);
    }

    public synchronized void addSubscription(String channel, int connectionId, int subscriptionId) {
        if (!channels.containsKey(channel))
            channels.put(channel, ConcurrentHashMap.newKeySet());
        if (!subscriptionIDs.containsKey(connectionId))
            subscriptionIDs.put(connectionId, new ConcurrentHashMap<>());
        channels.get(channel).add(connectionId);
        subscriptionIDs.get(connectionId).put(channel, subscriptionId);
    }

    public synchronized void removeSubscription(String topic, int connectionId) {
        channels.get(topic).remove(connectionId);
        subscriptionIDs.get(connectionId).remove(topic);
    }

    public synchronized String getPassword(String username) {
        if (!logins.containsKey(username))
            return null;
        return logins.get(username);
    }

    public int getMessageId() {
        return messageId++;
    }

    public synchronized Set<Integer> getChannel(String topic) {
        return channels.get(topic);
    }

    public synchronized Map<String, Integer> getSubscriptionIDs(int connectionId) {
        return subscriptionIDs.get(connectionId);
    }

    public synchronized void addLogin(String username, String password) {
        logins.put(username, password);
    }

    public boolean isActiveClient(String username) {
        return activeClientsNames.containsValue(username);
    }

    public void addActiveClientName(String username, int connectionId) {
        activeClientsNames.put(connectionId, username);
    }
}
