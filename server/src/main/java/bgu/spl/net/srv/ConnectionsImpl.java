package bgu.spl.net.srv;

import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

public class ConnectionsImpl<T> implements Connections<T> {
    // saves info about the active clients (login id, subscribed topics, etc.)

    Map<Integer, ConnectionHandler<T>> activeClients = new ConcurrentHashMap<>();
    Map<String, Set<Integer>> channels = new ConcurrentHashMap<>();

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> ch = activeClients.get(connectionId);
        if (ch != null) {
            ch.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg) {
        Set<Integer> channelList = channels.get(channel);
        if (channelList != null) {
            for (Integer id : channelList) 
                send(id, msg);
        }

    }

    @Override
    public void disconnect(int connectionId) {
        activeClients.remove(connectionId);
        for (Map.Entry<String, Set<Integer>> entry : channels.entrySet()) {
            // String key = entry.getKey();
            Set<Integer> value = entry.getValue();
            value.remove(connectionId);
        }
    }

    public Map<Integer, ConnectionHandler<T>> getActiveClients() {
        return activeClients;
    }

    public Map<String, Set<Integer>> getChannels() {
        return channels;
    }

    public void addActiveClient(int connectionId, ConnectionHandler<T> ch) {
        activeClients.put(connectionId, ch);
    }

    public void addChannel(String channel, int connectionId) {
        if (!channels.containsKey(channel)) {
            channels.put(channel, ConcurrentHashMap.newKeySet());
        }
        channels.get(channel).add(connectionId);
    }

    

}
