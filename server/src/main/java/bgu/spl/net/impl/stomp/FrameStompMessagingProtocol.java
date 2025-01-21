package bgu.spl.net.impl.stomp;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;

public class FrameStompMessagingProtocol implements StompMessagingProtocol<Frame> {

    private boolean shouldTerminate = false;

    private int connectionId; // Unique ID for this connection
    private Connections<Frame> connections; // Reference to manage connections

    private String username; // Current logged-in username (or null if not logged in)
    private String password; // Current logged-in password (or null if not logged in)
    private final Set<String> subscribedTopics = new HashSet<>(); // Topics this client subscribed to
    private final Map<Integer, String> receipts = new ConcurrentHashMap<>(); // To manage receipt IDs
    private final Map<Integer, String> subscriptions = new ConcurrentHashMap<>(); // To manage subscriptions


    @Override
    public void start(int connectionId, Connections<Frame> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(Frame message) {
        if(message.getCommand().equals("CONNECT")) {
            username = message.getHeaders().get("login");
            password = message.getHeaders().get("passcode");
        }if(message.getCommand().equals("SEND")) {
            // Do something with the message
        }if(message.getCommand().equals("SUBSCRIBE")) {
            String topic = message.getHeaders().get("destination");
            int subscriptionId = Integer.parseInt(message.getHeaders().get("id"));
            subscriptions.put(subscriptionId, topic);
            subscribedTopics.add(topic);
            ((ConnectionsImpl)connections).addChannel(topic, connectionId);
        }if(message.getCommand().equals("UNSUBSCRIBE")) {
            int subscriptionId = Integer.parseInt(message.getHeaders().get("id"));
            String topic = subscriptions.remove(subscriptionId);
            subscribedTopics.remove(topic);
        }if(message.getCommand().equals("DISCONNECT")) {

            // Do something with the message
            shouldTerminate = true;
        }
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
    
}
