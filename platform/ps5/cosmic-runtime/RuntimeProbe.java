import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.concurrent.FutureTask;

public final class RuntimeProbe {
    public static void run() throws Exception {
        System.out.println(System.getProperty("java.runtime.version"));
        Path marker = Path.of(System.getProperty("java.io.tmpdir"), "runtime-probe.txt");
        String previous = Files.exists(marker) ? Files.readString(marker) : "";
        if (!previous.matches("(?:java21\\n)*")) throw new AssertionError("Unexpected persistence marker");
        Files.writeString(marker, "java21\n", StandardOpenOption.CREATE, StandardOpenOption.APPEND);
        if (!Files.readString(marker).equals(previous + "java21\n")) throw new AssertionError("File round trip");
        System.out.println("File round trip passed; previous runs: " + previous.lines().count());

        FutureTask<Long> sum = new FutureTask<>(() -> {
            long value = 0;
            for (int i = 0; i <= 10_000; i++) value += i;
            return value;
        });
        Thread worker = new Thread(sum, "runtime-probe");
        worker.setDaemon(true);
        worker.start();
        worker.join(5000);
        if (worker.isAlive() || sum.get() != 50_005_000L) throw new AssertionError("Thread execution");
        System.out.println("Thread execution passed");

        try (ServerSocketChannel listener = ServerSocketChannel.open(); Selector selector = Selector.open()) {
            listener.configureBlocking(false);
            listener.bind(new InetSocketAddress(InetAddress.getLoopbackAddress(), 0));
            listener.register(selector, SelectionKey.OP_ACCEPT);
            try (Socket client = new Socket()) {
                client.connect(listener.getLocalAddress(), 5000);
                client.setSoTimeout(5000);
                if (selector.select(5000) == 0) throw new AssertionError("Selector timeout");
                try (Socket accepted = listener.accept().socket()) {
                    accepted.setSoTimeout(5000);
                    client.getOutputStream().write(83);
                    if (accepted.getInputStream().read() != 83) throw new AssertionError("Socket receive");
                    accepted.getOutputStream().write(21);
                    if (client.getInputStream().read() != 21) throw new AssertionError("Socket reply");
                }
            }
        }
        System.out.println("Loopback sockets and selector passed");
    }

    public static void main(String[] args) throws Exception { run(); }
}
