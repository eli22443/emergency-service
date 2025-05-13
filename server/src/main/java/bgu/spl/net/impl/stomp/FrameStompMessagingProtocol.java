package bgu.spl.net.impl.stomp;

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Arrays;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import bgu.spl.net.srv.ConnectionsImpl;

public class FrameStompMessagingProtocol implements StompMessagingProtocol<Frame> {

    private boolean shouldTerminate = false;
    private int connectionId; // Unique ID for this connection
    private ConnectionsImpl<Frame> connectionsImpl; // Reference to manage connections
    private final Set<String> subscribedTopics = new HashSet<>(); // Topics this client subscribed to
    private final Map<Integer, String> subscriptions = new ConcurrentHashMap<>(); // To manage subscriptions

    @Override
    public void start(int connectionId, Connections<Frame> connections) {
        this.connectionId = connectionId;
        this.connectionsImpl = (ConnectionsImpl<Frame>) connections;
    }

    @Override
    public void process(Frame message) {

        if (message.getCommand().equals("CONNECT")) {
            String username = message.getHeaders().get("login");
            String password = message.getHeaders().get("passcode");
            Map<String, String> headers = new HashMap<>();

            List<String> requiredHeaders = Arrays.asList(message.getHeaders().get("accept-version"),
                    message.getHeaders().get("host"), message.getHeaders().get("login"),
                    message.getHeaders().get("passcode"));
            for (String header : requiredHeaders) {
                if (header == null) {
                    errorLogin2(message);
                    shouldTerminate = true;
                    return;
                }
            }

            if (connectionsImpl.getPassword(username) != null) {
                if (!password.equals(connectionsImpl.getPassword(username))) {
                    errorLogin1(message, username);
                    shouldTerminate = true;
                } else {
                    if (connectionsImpl.isActiveClient(username)) {
                        errorLogin3(message, username);
                        shouldTerminate = true;
                        return;
                    }
                    connectionsImpl.addActiveClientName(username, connectionId);
                    headers.put("version", "1.2");
                    connectionsImpl.send(connectionId, new Frame("CONNECTED", headers, ""));
                }
            } else {
                connectionsImpl.addLogin(username, password);
                connectionsImpl.addActiveClientName(username, connectionId);
                headers.put("version", "1.2");
                connectionsImpl.send(connectionId, new Frame("CONNECTED", headers, ""));
            }
        }
        if (message.getCommand().equals("SEND")) {
            sendReceipt(message.getHeaders().get("receipt"));
            String topic = message.getHeaders().get("destination");
            String body = message.getBody();

            Map<String, String> headers = new ConcurrentHashMap<>();
            headers.put("message-id", connectionsImpl.getMessageId() + "");
            headers.put("destination", topic);

            for (Integer id : connectionsImpl.getChannels().get(topic)) {
                if (id == connectionId)
                    continue;
                headers.put("subscription", connectionsImpl.getSubscriptionIDs(id).get(topic) + "");
                connectionsImpl.send(id, new Frame("MESSAGE", headers, body));
            }

        }
        if (message.getCommand().equals("SUBSCRIBE")) {
            sendReceipt(message.getHeaders().get("receipt"));
            String topic = message.getHeaders().get("destination");
            int subscriptionId = Integer.parseInt(message.getHeaders().get("id"));
            subscriptions.put(subscriptionId, topic);
            subscribedTopics.add(topic);
            connectionsImpl.addSubscription(topic, connectionId, subscriptionId);
        }
        if (message.getCommand().equals("UNSUBSCRIBE")) {
            sendReceipt(message.getHeaders().get("receipt"));
            int subscriptionId = Integer.parseInt(message.getHeaders().get("id"));
            String topic = subscriptions.remove(subscriptionId);
            subscribedTopics.remove(topic);
            connectionsImpl.removeSubscription(topic, connectionId);

        }
        if (message.getCommand().equals("DISCONNECT")) {
            sendReceipt(message.getHeaders().get("receipt"));
            connectionsImpl.removeActiveClient(connectionId);
            shouldTerminate = true;
        }
    }

    public void sendReceipt(String receipt_id) {
        if (receipt_id != null) {
            Map<String, String> headers = new ConcurrentHashMap<>();
            headers.put("receipt-id", receipt_id);
            connectionsImpl.send(connectionId, new Frame("RECEIPT", headers, ""));
        }
    }

    public void errorLogin1(Frame message, String username) {// un- match password
        Map<String, String> errorHeaders = new ConcurrentHashMap<>();
        errorHeaders.put("message", "Password does not match username");
        connectionsImpl.send(connectionId, new Frame("ERROR", errorHeaders,
                "The message:\n----\n" + message.toString().substring(0, message.toString().length() - 2) + "----\n" +
                        "User " + username + "'s password is diffrent than what you inserted."));
    }

    public void errorLogin2(Frame message) {// one of the header is missing
        Map<String, String> errorHeaders = new ConcurrentHashMap<>();
        errorHeaders.put("message", "One of the header is missing ");
        connectionsImpl.send(connectionId, new Frame("ERROR", errorHeaders,
                "The message:\n----\n" + message.toString().substring(0, message.toString().length() - 2) + "----\n" +
                        " Check that you have entered all the required details."));
    }

    public void errorLogin3(Frame message, String username) {// if another client is already logged in
        Map<String, String> errorHeaders = new HashMap<>();
        errorHeaders.put("message", "User already logged in");
        connectionsImpl.send(connectionId, new Frame("ERROR", errorHeaders,
                "The message:\n----\n" + message.toString().substring(0, message.toString().length() - 2) + "----\n" +
                        "User " + username + " is already logged in somewhere else."));
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

}
