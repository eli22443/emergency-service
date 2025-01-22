package bgu.spl.net.impl.stomp;

import java.util.HashMap;
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

    // private String username=null; // Current logged-in username (or null if not logged in)
    // private String password=null; // Current logged-in password (or null if not logged in)
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
        ConnectionsImpl<Frame> connectionsImpl = (ConnectionsImpl<Frame>) connections;

        if(message.getCommand().equals("CONNECT")) {
            String username = message.getHeaders().get("login");
            String password = message.getHeaders().get("passcode");
            Map<String, String> headers = new HashMap<>();
            if(connectionsImpl.getPassword(username)==null || password.equals(connectionsImpl.getPassword(username))) {
                // Send CONNECTED frame
                headers.put("version", "1.2");
                connectionsImpl.send(connectionId, new Frame("CONNECTED", headers, ""));
            } else {
                // Send ERROR frame
                // headers.put("receipt - id", "message -12345");
                headers.put("message", "bad login");
                connectionsImpl.send(connectionId, new Frame("ERROR", headers, ""));
            }
        }if(message.getCommand().equals("SEND")) {
            // String receipt = message.getHeaders().get("receipt");
            // if(receipt != null) {
            //     Map<String, String> headers = new ConcurrentHashMap<>();
            //     headers.put("receipt-id", receipt);
            //     connections.send(connectionId, new Frame("RECEIPT", headers, ""));
            // }
            String topic = message.getHeaders().get("destination");
            String body = message.getBody();

            Map<String,String> headers = new ConcurrentHashMap<>();
            headers.put("message-id", connectionsImpl.getMessageId()+"");
            headers.put("destination", topic);

            for(Integer id: connectionsImpl.getChannels().get(topic)) {
                headers.put("subscription", connectionsImpl.getSubscriptionIDs(id).get(topic)+"");
                connectionsImpl.send(id, new Frame("MESSAGE", headers, body));
            }
            
        }if(message.getCommand().equals("SUBSCRIBE")) {
            String topic = message.getHeaders().get("destination");
            int subscriptionId = Integer.parseInt(message.getHeaders().get("id"));
            subscriptions.put(subscriptionId, topic);
            subscribedTopics.add(topic);
            connectionsImpl.addSubscription(topic, connectionId, subscriptionId);
        }if(message.getCommand().equals("UNSUBSCRIBE")) {
            // String receipt = message.getHeaders().get("receipt");
            // if(receipt != null) {
            //     Map<String, String> headers = new ConcurrentHashMap<>();
            //     headers.put("receipt-id", receipt);
            //     connections.send(connectionId, new Frame("RECEIPT", headers, ""));
            // }
            int subscriptionId = Integer.parseInt(message.getHeaders().get("id"));
            String topic = subscriptions.remove(subscriptionId);
            subscribedTopics.remove(topic);
            connectionsImpl.removeSubscription(topic, connectionId);

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
