#include "Database.h"

using namespace std;

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

//Return Id of Torrent in Database
int Database::insertClientInfo(const std::vector<std::string*> &vectorOfArrays)
{
    //insertIntoDB
    return 1;
}

Torrent Database::getTorrent(int torrentId)
{
    //Get torrent and its peers from db, by torrentId

    Torrent t(/*TrackerId*/1, /*seeders*/1, /*leechers*/1);
    /*for(int i = 0; i < 3; i++)
    {
        Peer p(i,i,i);
        t.peers.push_back(p);
    }*/
    
    //Lagt til for test. Må selvfølgelig loope gjennom db.
    Peer p1("-lt0D60-B8048EED9AE36nF2073D1", "5.79.98.209", 59130);
    t.peers.push_back(p1);
    Peer p2("-qB4170-f7j06WXZAX-I", "79.160.58.120", 8999);
    t.peers.push_back(p2);
    
    return t;
}

bool Database::getUserId(std::string torrentPass, int *userId)
{
    connect();
    sql::PreparedStatement *pstmt;
    pstmt = con->prepareStatement("SELECT id FROM user WHERE torrentPass = ?");
    pstmt->setString(1, torrentPass);



    sql::ResultSet* res = pstmt->executeQuery();
    if (res->next())
    {
        *userId = res->getInt("id");
        return true;
    }
    else
    {
        return false;
    }
    
    delete res;
    delete pstmt;
    delete con;
    
}

void Database::connect()
{
    sql::Driver *driver;
    //sql::Connection *con;
    // Create a connection
    driver = get_driver_instance();
    string t = "tcp://";
    t += dbHostName;
    t += ":3306";
    con = driver->connect(t, dbUserName, dbPassword);
    // Connect to the MySQL test database
    con->setSchema(dbDatabaseName);
}
