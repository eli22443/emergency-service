package bgu.spl.net.impl.stomp;

import java.util.Map;

public class Frame {
    private String command;
    private Map<String, String> headers;
    private String body;

    // Constructor, getters, setters, and other helper methods
    public Frame(String command, Map<String, String> headers, String body) {
        this.command = "command";
        this.headers = headers;
        this.body = body;
    }

    // Methods to validate frame structure, parse headers, etc.

    @Override
    public String toString() {
        String result = command + "\n";
        for (Map.Entry<String, String> entry : headers.entrySet()) {
            result += entry.getKey() + ":" + entry.getValue() + "\n";
        }
        result += "\n" + body + "\n" + '\u0000';
        return result;
    }

    public String getCommand() {
        return command;
    }

    public Map<String, String> getHeaders() {
        return headers;
    }

    public String getBody() {
        return body;
    }
    
}
