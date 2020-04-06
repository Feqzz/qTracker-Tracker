#include "Database.h"

Database::Database()
{
    /*
    try {
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        / Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        stmt = con->createStatement();
        res = stmt->executeQuery("SELECT username FROM user;");
        while (res->next()) {
            // Access column data by alias or column name
            cout << res->getString("username") << endl;
            // Access column data by numeric offset, 1 is the first column
            cout << res->getString(1) << endl;
        }
        delete res;
        delete stmt;
        delete con;

    } catch (sql::SQLException &e) {
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
    }
    */
}

int Database::parseEventString(std::string event)
{
    boost::algorithm::to_lower(event);
    if(event.compare("stopped") == 0)
    {
        return 3;
    }
    else if(event.compare("started") == 0)
    {
        return 2;
    }
    else if(event.compare("completed") == 0)
    {
        return 1;
    }
    else
    {
        std::cout << "event = Bump (0)" << std::endl;
        return 0;
    }
}

std::string Database::decode(std::string str)
{
    std::cout << std::endl << "-------- (Stian Decoder) --------" << std::endl << std::endl;
    std::string decoded = "";
    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] != '%')
        {
            std::ostringstream os;
            os << std::hex << (int)str[i];
            decoded += os.str();
            std::cout << str[i] << " ---> " << os.str() << std::endl;
        }
        else
        {
            decoded += std::tolower(str[i+1]);
            decoded += std::tolower(str[i+2]);
            i = i + 2;
        }
    }
    std::cout << "Size: " << decoded.size() << std::endl;
    std::cout << decoded << std::endl;
    std::cout << std::endl << "-------- (Stian Decoder) --------" << std::endl << std::endl;
    return decoded;
}

std::string Database::urlDecode(std::string urlEncodedString)
{
    //%8E%7C%7C%1Bt%2E%9D%C7%AE%7E%AA9%1CB%7D%19%14a%D6%0C
    std::string output = "";
    for(int i=0;i<urlEncodedString.length();i++)
    {
        if(urlEncodedString[i]=='%')
        {
            continue;
        }
        output+=urlEncodedString[i];
    }
    return output;
}

//Return infoHash if completed
std::string Database::insertClientInfo(const std::vector<std::string*> &vectorOfArrays)
{
    std::string ipa;
    int port;
    int event;
    std::string infoHash;
    std::string peerId;
    int64_t downloaded;
    int64_t left;
    int64_t uploaded;
    std::string torrentPass;
    for(size_t x=0;x<vectorOfArrays.size();x++)
    {
        if(vectorOfArrays.at(x)[0].compare("ip") == 0)
        {
            ipa = vectorOfArrays.at(x)[1];
            continue;
        }
        if(vectorOfArrays.at(x)[0].compare("port") == 0)
        {
            port = stoi(vectorOfArrays.at(x)[1]);
            continue;
        }
        if(vectorOfArrays.at(x)[0].compare("event") == 0)
        {
            std::cout << "event = " << vectorOfArrays.at(x)[1];
            event = parseEventString(vectorOfArrays.at(x)[1]);
            std::cout << " (" << event << ")" << std::endl;
            continue;
        }
        if(vectorOfArrays.at(x)[0].compare("info_hash") == 0)
        {
            infoHash = decode(vectorOfArrays.at(x)[1]);
            continue;
        }
        if(vectorOfArrays.at(x)[0].compare("peer_id") == 0)
        {
            peerId = vectorOfArrays.at(x)[1];
            continue;
        }
        if(vectorOfArrays.at(x)[0].compare("downloaded") == 0)
        {
            downloaded = stoll(vectorOfArrays.at(x)[1]);
            continue;
        }
        if(vectorOfArrays.at(x)[0].compare("left") == 0)
        {
            left = stoll(vectorOfArrays.at(x)[1]);
            continue;
        }
        if(vectorOfArrays.at(x)[0].compare("uploaded") == 0)
        {
            uploaded = stoll(vectorOfArrays.at(x)[1]);
            continue;
        }
        if(vectorOfArrays.at(x)[0].compare("urlKey") == 0)
        {
            torrentPass = vectorOfArrays.at(x)[1];
            continue;
        }
    }
    std::cout << torrentPass << std::endl;
    if (updateClientTorrents(ipa,port,event,infoHash,peerId,downloaded,left,uploaded,torrentPass))
    {
        return infoHash;
    }
    else
    {
        return NULL;
    }
    
}

std::vector<int> Database::getTorrentData(std::string infoHash)
{
    std::vector<int> vec;
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement("SELECT leechers, seeders FROM torrent WHERE infoHash = ?");
        pstmt->setString(1, infoHash);
        res = pstmt->executeQuery();
        if (res->next())
        {
            vec.push_back(res->getInt("seeders"));
            vec.push_back(res->getInt("leechers"));
            return vec;
        }
        else
        {
            std::cout << "Failed to get torrent Id" << std::endl;
            return vec;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::getTorrentId ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return vec;
    }
}

Torrent Database::getTorrent(std::string infoHash)
{
    std::vector<int> torrentData = getTorrentData(infoHash);

    Torrent t(/*TrackerId*/1, /*seeders*/torrentData[0], /*leechers*/torrentData[1]);

    t.peers = getPeers(infoHash);

    return t;
}

bool Database::getUserId(std::string torrentPass, int *userId)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement("SELECT id FROM user WHERE torrentPass = ?");
        pstmt->setString(1, torrentPass);
        res = pstmt->executeQuery();
        if (res->next())
        {
            *userId = res->getInt("id");
            std::cout << "Valid User ID" << std::endl;
            delete pstmt;
            delete res;
            return true;
        }
        else
        {
            std::cout << "Invalid User ID" << std::endl;
            delete pstmt;
            delete res;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::getUserId ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

}

bool Database::userCanLeech(int userId)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement("SELECT canLeech FROM user WHERE id = ?");
        pstmt->setInt(1, userId);
        res = pstmt->executeQuery();
        if (res->next())
        {
            std::cout << "User can leech" << std::endl;
            return (res->getInt("canLeech")) ? true : false;
        }
        else
        {
            std::cout << "User can't leech" << std::endl;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::userCanLeech ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

std::vector<Peer*> Database::getPeers(std::string infoHash)
{
    std::vector<Peer*> peers;
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement
                (
                    "SELECT "
                    "peerId,"
                    "ipa, "
                    "port "
                    "FROM "
                    "torrent AS t, "
                    "clientTorrents AS ct, "
                    "client AS c, "
                    "ipAddress AS ip "
                    "WHERE "
                    "t.id = ct.torrentId AND "
                    "ct.isActive = 1 AND "
                    "ct.clientId = c.id AND "
                    "c.ipaId = ip.id AND "
                    "t.infoHash = ?"
                    );
        pstmt->setString(1, infoHash);
        res = pstmt->executeQuery();
        while (res->next())
        {
            std::string peerId = res->getString("peerId");
            std::string ipa = res->getString("ipa");
            int port = res->getInt("port");
            peers.push_back(new Peer(peerId, ipa, port));
        }
        return peers;
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::getPeers ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return peers;
    }
}

bool Database::torrentExists(std::string infoHash, int uploaderUserId, int *torrentId, bool recursive)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement("SELECT id FROM torrent WHERE infoHash = ?");
        pstmt->setString(1, infoHash);
        res = pstmt->executeQuery();
        if (res->next())
        {
            *torrentId = res->getInt("id");
            std::cout << "Found existing torrent" << std::endl;
            return true;
        }
        else
        {

            
            std::cout << "Torrent doesn't exists. Something went wrong. info hash:"<< infoHash << std::endl;

           /* if (recursive)
            {
                return createTorrent(uploaderUserId, infoHash, torrentId);
            }
            else
            {
                return false;
            } */
            std::cout << "Torrent doesn't exists. ABORTING!" << std::endl;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::torrentExists ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool Database::createTorrent(int uploaderUserId, std::string infoHash, int *torrentId)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);
        pstmt = con->prepareStatement("INSERT INTO torrent (uploader, infoHash) VALUES (?, ?)");
        pstmt->setInt(1, uploaderUserId);
        pstmt->setString(2, infoHash);
        if (pstmt->executeQuery())
        {
            std::cout << "Created new Torrent. Will use recursive function to get torrentId" << std::endl;
            return torrentExists(infoHash, uploaderUserId, torrentId, false);
        }
        else
        {
            std::cout << "Failed to create new Torrent" << std::endl;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::createTorrent ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool Database::ipaIsBanned(std::string ipa)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);
        pstmt = con->prepareStatement("SELECT isBanned FROM ipAddress WHERE ipa = ?");
        pstmt->setString(1, ipa);
        res = pstmt->executeQuery();
        if(res->next())
        {
            int isBanned = res->getInt("isBanned");
            if (isBanned==0)
            {
                std::cout << "Valid IP Address" << std::endl;
                return false;
            }
            else
            {
                std::cout << "Banned IP Address " << std::endl;
                return true;
            }
        }
        else
        {
            std::cout << "IP Address does not exist, so user is not banned" << std::endl;
            return false;
        }
        
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::ipaIsBanned ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return true;
    }
}

bool Database::updateClientTorrents(std::string ipa, int port, int event, std::string infoHash, 
                                    std::string peerId, uint64_t downloaded, uint64_t left, uint64_t uploaded, std::string torrentPass)
{
    int userId, torrentId, clientId = -1;
    if (getUserId(torrentPass, &userId) && userCanLeech(userId) && !ipaIsBanned(ipa))
    {
        if(torrentExists(infoHash, userId, &torrentId, true))
        {
            if (getClientId(peerId, ipa, port, userId, &clientId, true))
            {
                try
                {
                    sql::Driver *driver;
                    sql::Connection *con;
                    sql::PreparedStatement *pstmt;
                    sql::ResultSet *res;

                    // Create a connection
                    driver = get_driver_instance();
                    std::string t = "tcp://";
                    t += dbHostName;
                    t += ":3306";
                    con = driver->connect(t, dbUserName, dbPassword);
                    // Connect to the MySQL test database
                    con->setSchema(dbDatabaseName);

                    pstmt = con->prepareStatement
                            (
                                "UPDATE clientTorrents SET "
                                "isActive = IF(? < 3, 1, 0), "
                                "announced = announced + 1, "
                                "completed = IF(? = 1, 1, 0), "
                                "downloaded = ?, "
                                "`left` = ?, "
                                "uploaded = ?, "
                                "lastEvent = ?, "
                                "lastActivity = NOW(), "
                                "timeActive = IF(isActive = 1, timeActive + TIMESTAMPDIFF(MINUTE, lastActivity, NOW()), timeActive) "
                                "WHERE torrentId = ? "
                                "AND clientId = ?"
                            );
                    pstmt->setInt(1, event);
                    pstmt->setInt(2, event);
                    pstmt->setUInt64(3, downloaded);
                    pstmt->setUInt64(4, left);
                    pstmt->setUInt64(5, uploaded);
                    pstmt->setInt(6, event);
                    pstmt->setInt(7, torrentId);
                    pstmt->setInt(8, clientId);
                    if (pstmt->executeUpdate() <= 0)
                    {
                        std::cout << "clientTorrent doesn't exist. Will create one. " << std::endl;
                        if (!createClientTorrent(torrentId, clientId, downloaded, left, uploaded, event))
                        {
                            return false;
                        }
                    }
                    if (!updateUserTorrentTotals(clientId,torrentId, userId, downloaded, uploaded))
                    {
                        return false;
                    }
                    return updateTorrent(torrentId, event);
                }
                catch (sql::SQLException &e)
                {
                    std::cout << "Database::updateClientTorrents ";
                    std::cout << " (MySQL error code: " << e.getErrorCode();
                    std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
                    return true;
                }
            }
        }
    }
    return false;
}

bool Database::createClientTorrent(int torrentId, int clientId, uint64_t downloaded, uint64_t left, uint64_t uploaded, int event)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement
                (
                    "INSERT INTO clientTorrents "
                    "(torrentId, clientId, downloaded, `left`, uploaded, lastEvent, lastActivity) "
                    "VALUES (?, ?, ?, ?, ?, ?, NOW())"
                    );
        pstmt->setInt(1, torrentId);
        pstmt->setInt(2, clientId);
        pstmt->setUInt64(3, downloaded);
        pstmt->setUInt64(4, left);
        pstmt->setUInt64(5, uploaded);
        pstmt->setInt(6, event);
        if (pstmt->executeQuery())
        {
            std::cout << "Created new clientTorrents " << std::endl;
            return true;
        }
        else
        {
            std::cout << "Failed creating new clientTorrents " << std::endl;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::createClientTorrent ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool Database::getIpaId(std::string ipa, int userId, int *ipaId, bool recursive)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement("SELECT id FROM ipAddress WHERE ipa = ?");
        pstmt->setString(1, ipa);
        res = pstmt->executeQuery();
        if (res->next())
        {
            *ipaId = res->getInt("id");
            std::cout << "Using known IP Address " << std::endl;
            return true;
        }
        else
        {
            std::cout << "Using unknown IP Adress. Will insert into database " << std::endl;
            if (recursive)
            {
                return createIpAddress(ipa, userId, ipaId);
            }
            else
            {
                return false;
            }
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::getIpaId ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool Database::getClientId(std::string peerId, std::string ipa, int port, int userId, int *clientId, bool recursive)
{
    int ipaId = -1;
    if (getIpaId(ipa, userId, &ipaId, true))
    {
        try
        {
            sql::Driver *driver;
            sql::Connection *con;
            sql::PreparedStatement *pstmt;
            sql::ResultSet *res;

            // Create a connection
            driver = get_driver_instance();
            std::string t = "tcp://";
            t += dbHostName;
            t += ":3306";
            con = driver->connect(t, dbUserName, dbPassword);
            // Connect to the MySQL test database
            con->setSchema(dbDatabaseName);

            pstmt = con->prepareStatement("SELECT id FROM client WHERE peerId = ? AND port = ? AND ipaId = ?");
            pstmt->setString(1, peerId);
            pstmt->setInt(2, port);
            pstmt->setInt(3, ipaId);
            res = pstmt->executeQuery();
            if (res->next())
            {
                *clientId = res->getInt("id");
                std::cout << "Using a known client" << std::endl;
                return true;
            }
            else
            {
                std::cout << "Client unknown. Will try to create one. " << std::endl;
                if (recursive)
                {
                    return createClient(peerId, ipa, port, ipaId, userId, clientId);
                }
                else
                {
                    return false;
                }
                
            }
        }
        catch (sql::SQLException &e)
        {
            std::cout << "Database::getClientId ";
            std::cout << " (MySQL error code: " << e.getErrorCode();
            std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "Should never end up here.." << std::endl;
        return false;
    }
    
}

bool Database::createIpAddress(std::string ipa, int userId, int *ipaId)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement("INSERT INTO ipAddress (ipa, userId) VALUES (?, ?)");
        pstmt->setString(1, ipa);
        pstmt->setInt(2, userId);
        if (pstmt->executeQuery())
        {
            std::cout << "IP Address added to Database. Will do recursive function to get ipaId" << std::endl;
            return getIpaId(ipa, userId, ipaId, false);
        }
        else
        {
            std::cout << "Failed adding IP Address to the Database " << std::endl;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::createIpAddress ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool Database::createClient(std::string peerId, std::string ipa, int port, int ipaId, int userId, int *clientId)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement("INSERT IGNORE INTO client (peerId, port, ipaId) VALUES (?, ?, ?)");
        pstmt->setString(1, peerId);
        pstmt->setInt(2, port);
        pstmt->setInt(3, ipaId);
        if (pstmt->executeQuery())
        {
            std::cout << "Added new client to the Database. Will do recursive function to get clientId" << std::endl;
            return getClientId(peerId, ipa, port, userId, clientId, false);
        }
        else
        {
            std::cout << "Failed adding new client to the Database " << std::endl;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::createClient ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool Database::updateUserTorrentTotals(int clientId,int torrentId, int userId, uint64_t downloaded, uint64_t uploaded)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement
                (
                    "SELECT downloaded,uploaded FROM "
                    "clientTorrents "
                    "WHERE torrentId = ? "
                    "AND clientId = ?"
                    );
        pstmt->setInt(1, torrentId);
        pstmt->setInt(2, clientId);
        res = pstmt->executeQuery();
        if(res->next())
        {
            uint64_t oldDownloaded = res->getUInt64("downloaded");
            uint64_t oldUploaded = res->getUInt64("uploaded");

            uint64_t downloadedTotalIncrement = downloaded-oldDownloaded;
            uint64_t uploadedTotalIncrement = uploaded-oldUploaded;

            //Checks if torrentId,userId combo exists before update
            sql::PreparedStatement *pstmt3 = con->prepareStatement
                    (
                        "SELECT 1 FROM "
                        "userTorrentTotals "
                        "WHERE torrentId = ? "
                        "AND userId = ?"
                        );
            pstmt3->setInt(1, torrentId);
            pstmt3->setInt(2, userId);
            sql::ResultSet *res2 = pstmt3->executeQuery();
            if(res2->next())
            {
                int boolTest = res2->getInt("1");
                if(boolTest==1)
                {

                    /* If user har restartet the same torrent we do not
                    want to update the total with a negative number */
                    if(downloadedTotalIncrement<=0)
                    {
                        downloadedTotalIncrement = downloaded;
                    }
                    if(uploadedTotalIncrement<=0)
                    {
                        uploadedTotalIncrement = uploaded;
                    }
                    std::string query = "UPDATE userTorrentTotals SET "
                                        "totalDownloaded = totalDownloaded + ?,"
                                        "totalUploaded = totalUploaded + ? "
                                        "WHERE torrentId = ? AND userId = ?;";
                    //std::cout << "query: \n" << query << std::endl;
                    sql::PreparedStatement *pstmt2;
                    pstmt2 = con->prepareStatement(query);
                    pstmt2->setUInt64(1, downloadedTotalIncrement);
                    pstmt2->setUInt64(2, uploadedTotalIncrement);
                    pstmt2->setInt(3, torrentId);
                    pstmt2->setInt(4, userId);
                    pstmt2->executeUpdate();
                    std::cout << "Updated userTorrentTotals in the Database" << std::endl;
                    return true;
                }               
            }
        }
        std::cout << "Failed to update userTorrentTotals in the Database. Will create one " << std::endl;
        return createUserTorrentTotals(torrentId, userId, downloaded, uploaded);
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::updateUserTorrentTotals ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool Database::createUserTorrentTotals(int torrentId, int userId, uint64_t downloaded, uint64_t uploaded)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);

        pstmt = con->prepareStatement("INSERT INTO userTorrentTotals (torrentId, userId, totalDownloaded, totalUploaded) VALUES (?, ?, ?, ?)");
        pstmt->setInt(1, torrentId);
        pstmt->setInt(2, userId);
        pstmt->setUInt64(3, downloaded);
        pstmt->setUInt64(4, uploaded);
        if (pstmt->executeQuery())
        {
            std::cout << "Added new userTorrentTotals to the Database" << std::endl;
            return true;
        }
        else
        {
            std::cout << "Failed adding new userTorrentTotals to the Database " << std::endl;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::createUserTorrentTotals ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool Database::updateTorrent(int torrentId, int event)
{
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::PreparedStatement *pstmt;
        sql::ResultSet *res;

        // Create a connection
        driver = get_driver_instance();
        std::string t = "tcp://";
        t += dbHostName;
        t += ":3306";
        con = driver->connect(t, dbUserName, dbPassword);
        // Connect to the MySQL test database
        con->setSchema(dbDatabaseName);
        bool isCompleted = (event == 1) ? true : false;

        pstmt = con->prepareStatement
                (
                    "UPDATE torrent "
                    "SET "
                    "seeders = (SELECT SUM(isActive) FROM clientTorrents WHERE torrentId = ?), "
                    "leechers = (SELECT SUM(isActive) FROM clientTorrents WHERE completed = 0 AND torrentId = ?), "
                    "completed = IF(? = 1, completed + 1, completed) "
                    "WHERE id = ?;"
                    );
        pstmt->setInt(1, torrentId);
        pstmt->setInt(2, torrentId);
        pstmt->setBoolean(3, isCompleted);
        pstmt->setInt(4, torrentId);
        if (pstmt->executeQuery())
        {
            std::cout << "Torrent updated" << std::endl;
            return true;
        }
        else
        {
            std::cout << "Failed to update Torrent" << std::endl;
            return false;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cout << "Database::updateTorrent ";
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

